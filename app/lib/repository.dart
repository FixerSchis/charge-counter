import 'dart:async';
import 'dart:convert';

import 'package:flutter/widgets.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';
import 'package:shared_preferences/shared_preferences.dart';

import 'gatt.dart';
import 'models.dart';
import 'platform_info.dart';

class _Session {
  _Session(this.device);

  final BluetoothDevice device;
  BluetoothCharacteristic? count;
  BluetoothCharacteristic? max;
  BluetoothCharacteristic? name;
  BluetoothCharacteristic? locate;
  BluetoothCharacteristic? battery;
  StreamSubscription<BluetoothConnectionState>? connectionSub;
  final List<StreamSubscription<List<int>>> valueSubs = [];

  bool get ready => count != null && max != null && name != null && locate != null;
}

class CounterScope extends InheritedNotifier<CounterRepository> {
  const CounterScope({
    super.key,
    required CounterRepository repository,
    required super.child,
  }) : super(notifier: repository);

  static CounterRepository of(BuildContext context) {
    final scope = context.dependOnInheritedWidgetOfExactType<CounterScope>();
    assert(scope != null, 'CounterScope is missing');
    return scope!.notifier!;
  }
}

/// Bonded counters this install knows, plus the pairing-window scan.
class CounterRepository extends ChangeNotifier with WidgetsBindingObserver {
  static const _storageKey = 'bonded_counters_v1';
  static const _explainedKey = 'bluetooth_explained_v1';
  static const _maxLinks = 6;

  final Map<String, CachedCounter> _counters = {};
  final Map<String, PairingOffer> _offers = {};
  final Map<String, _Session> _sessions = {};
  final Map<String, Future<void>> _inflight = {};
  final Map<String, DateTime> _retryAfter = {};
  final Map<String, String> _connectErrors = {};
  final Set<String> _busy = {};

  StreamSubscription<List<ScanResult>>? _scanSub;
  StreamSubscription<BluetoothAdapterState>? _adapterSub;
  bool _started = false;
  bool _loaded = false;
  bool _sawAdvertisement = false;
  bool _bonding = false;
  bool _saveQueued = false;
  DateTime? _lastSave;

  BluetoothAdapterState adapterState = BluetoothAdapterState.unknown;
  bool explainsLocation = false;
  bool locationServicesOff = false;
  bool permissionBlocked = false;
  String? scanError;

  List<CachedCounter> get counters {
    final list = _counters.values.toList();
    list.sort((a, b) => a.label.toLowerCase().compareTo(b.label.toLowerCase()));
    return list;
  }

  List<PairingOffer> get pairingOffers {
    final list = _offers.values.toList();
    list.sort((a, b) => a.label.toLowerCase().compareTo(b.label.toLowerCase()));
    return list;
  }

  bool isBusy(String id) => _busy.contains(id);

  String? connectErrorFor(String id) => _connectErrors[id];

  CachedCounter? find(String id) => _counters[id];

  bool get removesSystemBond => PlatformInfo.isAndroid;

  Future<void> load() async {
    if (_loaded) return;
    _loaded = true;
    final prefs = await SharedPreferences.getInstance();
    final raw = prefs.getString(_storageKey);
    if (raw != null) {
      final decoded = jsonDecode(raw);
      if (decoded is List) {
        for (final entry in decoded) {
          if (entry is Map<String, dynamic>) {
            final counter = CachedCounter.fromJson(entry);
            if (counter != null) _counters[counter.id] = counter;
          } else if (entry is Map) {
            final counter = CachedCounter.fromJson(Map<String, dynamic>.from(entry));
            if (counter != null) _counters[counter.id] = counter;
          }
        }
      }
    }
    notifyListeners();
  }

  Future<bool> needsExplanation() async {
    if (!PlatformInfo.isAndroid) {
      final prefs = await SharedPreferences.getInstance();
      return !(prefs.getBool(_explainedKey) ?? false);
    }
    final sdk = await PlatformInfo.androidSdkInt();
    explainsLocation = sdk <= 30;
    final scan = await Permission.bluetoothScan.status;
    final connect = await Permission.bluetoothConnect.status;
    if (!scan.isGranted || !connect.isGranted) return true;
    if (explainsLocation) {
      final location = await Permission.locationWhenInUse.status;
      if (!location.isGranted) return true;
    }
    return false;
  }

  Future<void> markExplained() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setBool(_explainedKey, true);
  }

  /// Shows the system prompt. Call only after the in-app explanation.
  Future<bool> requestAccess() async {
    await markExplained();
    if (!PlatformInfo.isAndroid) return true;
    final sdk = await PlatformInfo.androidSdkInt();
    explainsLocation = sdk <= 30;
    final permissions = <Permission>[
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      if (explainsLocation) Permission.locationWhenInUse,
    ];
    final results = await permissions.request();
    final granted = results.values.every((status) => status.isGranted);
    permissionBlocked = !granted;
    notifyListeners();
    return granted;
  }

  Future<void> start() async {
    await load();
    if (_started) {
      await _startScan();
      return;
    }
    _started = true;
    WidgetsBinding.instance.addObserver(this);
    _scanSub = FlutterBluePlus.onScanResults.listen(_onScan, onError: _handleScanError);
    _adapterSub = FlutterBluePlus.adapterState.listen((state) {
      adapterState = state;
      if (state == BluetoothAdapterState.on) {
        unawaited(_startScan());
      }
      notifyListeners();
    });
    adapterState = FlutterBluePlus.adapterStateNow;
    if (adapterState == BluetoothAdapterState.on) {
      await _startScan();
    }
    notifyListeners();
  }

  Future<void> retryScan() async {
    scanError = null;
    locationServicesOff = false;
    permissionBlocked = false;
    notifyListeners();
    await _startScan();
  }

  Future<void> bond(String id) async {
    final offer = _offers[id];
    if (offer == null) {
      throw const CounterEditException('That counter left pairing mode.');
    }
    final existing = _inflight[id];
    if (existing != null) {
      await existing;
      return;
    }
    _busy.add(id);
    _bonding = true;
    notifyListeners();
    final future = _connectAndSync(
      BluetoothDevice.fromId(id),
      createBond: true,
      advName: offer.advName,
      report: true,
    );
    _inflight[id] = future;
    try {
      await future;
    } finally {
      _inflight.remove(id);
      _bonding = false;
      _busy.remove(id);
      notifyListeners();
    }
  }

  Future<void> ensureConnected(String id, {required bool report}) {
    final session = _sessions[id];
    if (session != null && session.ready && session.device.isConnected) {
      return Future<void>.value();
    }
    final existing = _inflight[id];
    if (existing != null) return existing;
    final future = _connectAndSync(
      BluetoothDevice.fromId(id),
      createBond: false,
      advName: null,
      report: report,
    );
    _inflight[id] = future;
    return future.whenComplete(() => _inflight.remove(id));
  }

  Future<void> writeMax(String id, int value) async {
    if (value < maxLower || value > maxUpper) {
      throw const CounterEditException('Maximum must be from 1 to 16.');
    }
    final session = await _require(id);
    final characteristic = session.max;
    if (characteristic == null) {
      throw const CounterEditException('Not connected to this counter.');
    }
    try {
      await characteristic.write(encodeUint16(value));
      final reported = decodeUint16(await characteristic.read(timeout: 20));
      if (reported == null) {
        throw const CounterEditException('The counter rejected that maximum.');
      }
      _applyMax(id, reported);
      final countChar = session.count;
      if (countChar != null) {
        final count = decodeUint16(await countChar.read(timeout: 20));
        if (count != null) _applyCount(id, count);
      }
      if (reported != value) {
        throw CounterEditException('The counter kept maximum $reported.');
      }
    } on CounterEditException {
      rethrow;
    } catch (error) {
      throw CounterEditException(_writeMessage(error, 'maximum'));
    }
  }

  Future<void> writeName(String id, String name) async {
    final bytes = utf8.encode(name);
    if (bytes.length > nameMaxBytes) {
      throw const CounterEditException('The name must be 32 bytes of UTF-8 or fewer.');
    }
    final session = await _require(id);
    final characteristic = session.name;
    if (characteristic == null) {
      throw const CounterEditException('Not connected to this counter.');
    }
    try {
      await characteristic.write(bytes, allowLongWrite: true);
      final reported = tryDecodeName(await characteristic.read(timeout: 20));
      if (reported == null) {
        throw const CounterEditException('The counter rejected that name.');
      }
      _applyName(id, reported);
      if (reported != name) {
        throw CounterEditException(
          reported.isEmpty ? 'The counter cleared the name.' : 'The counter kept the name "$reported".',
        );
      }
    } on CounterEditException {
      rethrow;
    } catch (error) {
      throw CounterEditException(_writeMessage(error, 'name'));
    }
  }

  Future<void> locate(String id) async {
    final session = await _require(id);
    final characteristic = session.locate;
    if (characteristic == null) {
      throw const CounterEditException('Not connected to this counter.');
    }
    try {
      await characteristic.write([0x01]);
    } catch (error) {
      throw CounterEditException(_writeMessage(error, 'locate'));
    }
  }

  Future<void> forget(String id) async {
    final device = _sessions[id]?.device ?? BluetoothDevice.fromId(id);
    _busy.add(id);
    notifyListeners();
    try {
      if (device.isDisconnected) {
        try {
          await device.connect(license: License.nonprofit, timeout: const Duration(seconds: 8));
        } catch (_) {
          // The local bond can still be dropped when the counter is out of range.
        }
      }
      if (PlatformInfo.isAndroid) {
        await device.removeBond();
      }
      await _closeSession(id);
      _counters.remove(id);
      _offers.remove(id);
      _connectErrors.remove(id);
      _retryAfter.remove(id);
      await _saveNow();
    } finally {
      _busy.remove(id);
      notifyListeners();
    }
  }

  @override
  void didChangeAppLifecycleState(AppLifecycleState state) {
    if (state == AppLifecycleState.resumed) {
      unawaited(_startScan());
    } else if (state == AppLifecycleState.paused) {
      unawaited(_sleepRadios());
    }
  }

  @override
  void dispose() {
    WidgetsBinding.instance.removeObserver(this);
    unawaited(_scanSub?.cancel());
    unawaited(_adapterSub?.cancel());
    unawaited(FlutterBluePlus.stopScan());
    for (final id in _sessions.keys.toList()) {
      unawaited(_closeSession(id));
    }
    super.dispose();
  }

  Future<void> _startScan() async {
    if (!_started || adapterState != BluetoothAdapterState.on) return;
    if (FlutterBluePlus.isScanningNow) return;
    _sawAdvertisement = false;
    final checkLocation = explainsLocation || (PlatformInfo.isAndroid && await PlatformInfo.androidSdkInt() <= 30);
    explainsLocation = checkLocation;
    try {
      await FlutterBluePlus.startScan(
        withServices: [ChargeUuids.service],
        continuousUpdates: true,
        removeIfGone: const Duration(seconds: 8),
        androidUsesFineLocation: false,
        androidCheckLocationServices: checkLocation,
        androidScanMode: AndroidScanMode.lowLatency,
      );
      scanError = null;
      locationServicesOff = false;
      permissionBlocked = false;
    } catch (error) {
      _handleScanError(error);
    }
    notifyListeners();
  }

  Future<void> _sleepRadios() async {
    try {
      await FlutterBluePlus.stopScan();
    } catch (_) {}
    if (_bonding) return;
    for (final id in _sessions.keys.toList()) {
      await _closeSession(id);
      final counter = _counters[id];
      if (counter != null) counter.connected = false;
    }
    notifyListeners();
  }

  void _onScan(List<ScanResult> results) {
    if (results.isEmpty) {
      if (!_sawAdvertisement) return;
    } else {
      _sawAdvertisement = true;
    }

    final nextOffers = <String, PairingOffer>{};
    final seen = <String>{};
    var changed = false;
    final now = DateTime.now();

    for (final result in results) {
      final id = result.device.remoteId.str;
      seen.add(id);
      final advName = result.advertisementData.advName;
      final known = _counters[id];
      if (known != null) {
        if (advName.isNotEmpty && known.advName != advName) {
          known.advName = advName;
          changed = true;
        }
        if (!known.inRange) {
          known.inRange = true;
          changed = true;
        }
        known.lastSeen = now;
        changed = true;
        _noteSeen(id);
      } else if (advertisementIsPairing(result.advertisementData)) {
        nextOffers[id] = PairingOffer(id: id, advName: advName);
      }
    }

    for (final counter in _counters.values) {
      if (counter.connected) {
        if (!counter.inRange) {
          counter.inRange = true;
          changed = true;
        }
        continue;
      }
      if (!seen.contains(counter.id) && counter.inRange) {
        counter.inRange = false;
        changed = true;
      }
    }

    final offersChanged = nextOffers.length != _offers.length ||
        nextOffers.keys.any((id) => _offers[id]?.advName != nextOffers[id]?.advName);
    _offers
      ..clear()
      ..addAll(nextOffers);

    if (changed) _scheduleSave();
    if (changed || offersChanged) notifyListeners();
  }

  void _noteSeen(String id) {
    final counter = _counters[id];
    if (counter == null || counter.connected || _inflight.containsKey(id)) return;
    final retry = _retryAfter[id];
    if (retry != null && DateTime.now().isBefore(retry)) return;
    if (_sessions.length + _inflight.length >= _maxLinks) return;
    unawaited(ensureConnected(id, report: false).catchError((_) {}));
  }

  Future<void> _connectAndSync(
    BluetoothDevice device, {
    required bool createBond,
    required String? advName,
    required bool report,
  }) async {
    final id = device.remoteId.str;
    final current = _sessions[id];
    if (!createBond && current != null && current.ready && current.device.isConnected) {
      return;
    }

    await _closeSession(id);
    StreamSubscription<BluetoothConnectionState>? connectionSub;
    try {
      var sawConnected = device.isConnected;
      connectionSub = device.connectionState.listen((state) {
        if (state == BluetoothConnectionState.connected) {
          sawConnected = true;
          return;
        }
        if (!sawConnected || state != BluetoothConnectionState.disconnected) return;
        final session = _sessions[id];
        if (session == null || session.device != device) return;
        _sessions.remove(id);
        for (final sub in session.valueSubs) {
          unawaited(sub.cancel());
        }
        final counter = _counters[id];
        if (counter != null && counter.connected) {
          counter.connected = false;
          notifyListeners();
        }
        scheduleMicrotask(() => connectionSub?.cancel());
      });

      if (device.isDisconnected) {
        await device.connect(license: License.nonprofit, timeout: const Duration(seconds: 20));
      }
      if (createBond && PlatformInfo.isAndroid) {
        final bond = await device.bondState.first;
        if (bond != BluetoothBondState.bonded) {
          await device.createBond();
        }
        _remember(id, advName);
      }
      if (device.isDisconnected) {
        throw const CounterEditException(
          'The counter did not stay connected. Hold the button until the lamps blink blue, then try again.',
        );
      }

      final services = await device.discoverServices();
      final session = _Session(device);
      session.connectionSub = connectionSub;
      session.count = _find(services, ChargeUuids.service, ChargeUuids.count);
      session.max = _find(services, ChargeUuids.service, ChargeUuids.max);
      session.name = _find(services, ChargeUuids.service, ChargeUuids.name);
      session.locate = _find(services, ChargeUuids.service, ChargeUuids.locate);
      session.battery = _find(services, ChargeUuids.batteryService, ChargeUuids.batteryLevel);
      if (!session.ready) {
        throw const CounterEditException('This device does not look like a Charge Counter.');
      }

      _attachListeners(session);
      _sessions[id] = session;

      final count = decodeUint16(await session.count!.read(timeout: 30));
      if (count == null) {
        throw const CounterEditException('The counter did not return its count.');
      }
      _remember(id, advName);
      _applyCount(id, count);

      await session.count!.setNotifyValue(true);
      await session.max!.setNotifyValue(true);
      await session.name!.setNotifyValue(true);

      final maximum = decodeUint16(await session.max!.read(timeout: 20));
      if (maximum != null) _applyMax(id, maximum);
      final label = tryDecodeName(await session.name!.read(timeout: 20));
      if (label != null) _applyName(id, label);
      final battery = session.battery;
      if (battery != null) {
        try {
          final level = await battery.read(timeout: 15);
          if (level.isNotEmpty) _applyBattery(id, level.first);
        } catch (_) {}
      }

      final counter = _counters[id];
      if (counter != null) {
        counter.connected = device.isConnected;
        counter.inRange = true;
        counter.lastSeen = DateTime.now();
      }
      _connectErrors.remove(id);
      _retryAfter.remove(id);
      await _saveNow();
      notifyListeners();
    } catch (error) {
      await connectionSub?.cancel();
      final session = _sessions.remove(id);
      if (session != null) {
        for (final sub in session.valueSubs) {
          await sub.cancel();
        }
      }
      if (device.isConnected) {
        try {
          await device.disconnect();
        } catch (_) {}
      }
      final counter = _counters[id];
      if (counter != null) {
        counter.connected = false;
        await _saveNow();
      }
      _retryAfter[id] = DateTime.now().add(const Duration(seconds: 12));
      if (report) {
        _connectErrors[id] = _connectMessage(error);
        notifyListeners();
      }
      if (error is CounterEditException) rethrow;
      throw CounterEditException(_connectMessage(error));
    }
  }

  Future<_Session> _require(String id) async {
    await ensureConnected(id, report: true);
    final session = _sessions[id];
    if (session == null || !session.ready || session.device.isDisconnected) {
      throw const CounterEditException('Not connected to this counter.');
    }
    return session;
  }

  void _attachListeners(_Session session) {
    final id = session.device.remoteId.str;
    void listen(BluetoothCharacteristic? characteristic, void Function(List<int> value) apply) {
      if (characteristic == null) return;
      final sub = characteristic.onValueReceived.listen(apply);
      session.valueSubs.add(sub);
      session.device.cancelWhenDisconnected(sub, delayed: true);
    }

    listen(session.count, (value) {
      final count = decodeUint16(value);
      if (count != null) _applyCount(id, count);
    });
    listen(session.max, (value) {
      final maximum = decodeUint16(value);
      if (maximum != null) _applyMax(id, maximum);
    });
    listen(session.name, (value) {
      final label = tryDecodeName(value);
      if (label != null) _applyName(id, label);
    });
  }

  BluetoothCharacteristic? _find(List<BluetoothService> services, Guid serviceId, Guid characteristicId) {
    for (final service in services) {
      if (service.uuid != serviceId) continue;
      for (final characteristic in service.characteristics) {
        if (characteristic.uuid == characteristicId) return characteristic;
      }
    }
    return null;
  }

  void _remember(String id, String? advName) {
    final counter = _counters.putIfAbsent(id, () => CachedCounter(id: id));
    if (advName != null && advName.isNotEmpty) counter.advName = advName;
    counter.inRange = true;
    counter.lastSeen ??= DateTime.now();
    _offers.remove(id);
  }

  void _applyCount(String id, int count) {
    final counter = _counters[id];
    if (counter == null || counter.count == count) return;
    counter.count = count;
    counter.lastSeen = DateTime.now();
    _scheduleSave();
    notifyListeners();
  }

  void _applyMax(String id, int maximum) {
    final counter = _counters[id];
    if (counter == null || counter.max == maximum) return;
    counter.max = maximum;
    counter.lastSeen = DateTime.now();
    _scheduleSave();
    notifyListeners();
  }

  void _applyName(String id, String name) {
    final counter = _counters[id];
    if (counter == null || counter.name == name) return;
    counter.name = name;
    counter.lastSeen = DateTime.now();
    _scheduleSave();
    notifyListeners();
  }

  void _applyBattery(String id, int level) {
    final counter = _counters[id];
    if (counter == null) return;
    final clamped = level.clamp(0, 100);
    if (counter.battery == clamped) return;
    counter.battery = clamped;
    counter.lastSeen = DateTime.now();
    _scheduleSave();
    notifyListeners();
  }

  Future<void> _closeSession(String id) async {
    final session = _sessions.remove(id);
    if (session == null) return;
    await session.connectionSub?.cancel();
    for (final sub in session.valueSubs) {
      await sub.cancel();
    }
    if (session.device.isConnected) {
      try {
        await session.device.disconnect();
      } catch (_) {}
    }
  }

  void _handleScanError(Object error) {
    final text = error.toString();
    if (text.contains('Location services are required')) {
      locationServicesOff = true;
      scanError = null;
    } else if (text.contains('Permission')) {
      permissionBlocked = true;
      scanError = null;
    } else if (text.contains('Bluetooth must be turned on')) {
      scanError = null;
    } else {
      scanError = 'Scanning failed. Try again.';
    }
    notifyListeners();
  }

  String _connectMessage(Object error) {
    if (error is CounterEditException) return error.message;
    final text = error.toString().toLowerCase();
    if (text.contains('bond') || text.contains('pair')) {
      return 'Pairing did not finish. Hold the button until the lamps blink blue, then try again.';
    }
    if (text.contains('timeout') || text.contains('disconnect')) {
      return 'The counter did not stay connected. Move closer and try again.';
    }
    return 'Could not connect to this counter.';
  }

  String _writeMessage(Object error, String what) {
    final text = error.toString().toLowerCase();
    if (text.contains('timeout') || text.contains('disconnect')) {
      return 'The counter disconnected before the $what was saved.';
    }
    return 'The counter rejected that $what.';
  }

  void _scheduleSave() {
    if (_saveQueued) return;
    _saveQueued = true;
    final elapsed = _lastSave == null ? const Duration(days: 1) : DateTime.now().difference(_lastSave!);
    final wait = elapsed >= const Duration(seconds: 5) ? Duration.zero : const Duration(seconds: 5) - elapsed;
    unawaited(Future<void>.delayed(wait, () async {
      _saveQueued = false;
      await _saveNow();
    }));
  }

  Future<void> _saveNow() async {
    _lastSave = DateTime.now();
    final prefs = await SharedPreferences.getInstance();
    final payload = jsonEncode(_counters.values.map((counter) => counter.toJson()).toList());
    await prefs.setString(_storageKey, payload);
  }
}
