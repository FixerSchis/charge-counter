import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

import 'repository.dart';
import 'screens/device_list_screen.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  FlutterBluePlus.setOperationQueueMode(OperationQueueMode.perDevice);
  runApp(const ChargeCounterApp());
}

class ChargeCounterApp extends StatefulWidget {
  const ChargeCounterApp({super.key});

  @override
  State<ChargeCounterApp> createState() => _ChargeCounterAppState();
}

class _ChargeCounterAppState extends State<ChargeCounterApp> {
  final CounterRepository _repository = CounterRepository();
  _Gate _gate = _Gate.checking;

  @override
  void initState() {
    super.initState();
    _prepare();
  }

  @override
  void dispose() {
    _repository.dispose();
    super.dispose();
  }

  Future<void> _prepare() async {
    await _repository.load();
    final explain = await _repository.needsExplanation();
    if (!mounted) return;
    if (!explain) {
      await _repository.start();
      if (!mounted) return;
      setState(() => _gate = _Gate.ready);
    } else {
      setState(() => _gate = _Gate.explain);
    }
  }

  bool _requesting = false;

  Future<void> _continue() async {
    if (_requesting) return;
    _requesting = true;
    try {
      final granted = await _repository.requestAccess();
      if (!mounted) return;
      if (!granted) {
        setState(() => _gate = _Gate.denied);
        return;
      }
      await _repository.start();
      if (!mounted) return;
      setState(() => _gate = _Gate.ready);
    } finally {
      _requesting = false;
    }
  }

  Future<void> _retryDenied() async {
    final explain = await _repository.needsExplanation();
    if (!mounted) return;
    if (!explain) {
      await _repository.start();
      if (!mounted) return;
      setState(() => _gate = _Gate.ready);
      return;
    }
    await _continue();
  }

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Charge Counter',
      theme: _theme(),
      home: CounterScope(
        repository: _repository,
        child: switch (_gate) {
          _Gate.checking => const Scaffold(body: Center(child: CircularProgressIndicator())),
          _Gate.explain => _Explanation(
              explainsLocation: _repository.explainsLocation,
              onContinue: () {
                unawaited(_continue());
              },
            ),
          _Gate.denied => _Denied(
              onSettings: openAppSettings,
              onRetry: () {
                unawaited(_retryDenied());
              },
            ),
          _Gate.ready => const DeviceListScreen(),
        },
      ),
    );
  }
}

enum _Gate { checking, explain, denied, ready }

class _Explanation extends StatelessWidget {
  const _Explanation({required this.explainsLocation, required this.onContinue});

  final bool explainsLocation;
  final VoidCallback onContinue;

  @override
  Widget build(BuildContext context) {
    final muted = Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.75);
    final body = explainsLocation
        ? 'Charge Counter uses Bluetooth to find a counter you put into pairing mode, and to read counters this phone has bonded. This version of Android also asks for location permission before it allows a Bluetooth scan. The app does not record where you are.'
        : 'Charge Counter uses Bluetooth to find a counter you put into pairing mode, and to read counters this phone has bonded. Counters paired with another phone are not listed. The app does not use Bluetooth to find your location.';
    return Scaffold(
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.fromLTRB(24, 48, 24, 24),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text('Bluetooth', style: Theme.of(context).textTheme.headlineMedium),
              const SizedBox(height: 16),
              Text(body, style: TextStyle(color: muted, height: 1.4, fontSize: 16)),
              const Spacer(),
              FilledButton(onPressed: onContinue, child: const Text('Continue')),
            ],
          ),
        ),
      ),
    );
  }
}

class _Denied extends StatelessWidget {
  const _Denied({required this.onSettings, required this.onRetry});

  final VoidCallback onSettings;
  final VoidCallback onRetry;

  @override
  Widget build(BuildContext context) {
    final muted = Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.75);
    return Scaffold(
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.fromLTRB(24, 48, 24, 24),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text('Bluetooth is off', style: Theme.of(context).textTheme.headlineMedium),
              const SizedBox(height: 16),
              Text(
                'Charge Counter needs Bluetooth permission to find a counter in pairing mode and to read the ones this phone has bonded.',
                style: TextStyle(color: muted, height: 1.4, fontSize: 16),
              ),
              const Spacer(),
              FilledButton(onPressed: onSettings, child: const Text('Open settings')),
              const SizedBox(height: 12),
              OutlinedButton(onPressed: onRetry, child: const Text('Try again')),
            ],
          ),
        ),
      ),
    );
  }
}

ThemeData _theme() {
  const ink = Color(0xFF1C1915);
  const paper = Color(0xFFF3EFE6);
  const card = Color(0xFFFFFCF7);
  const accent = Color(0xFFC4512C);
  const scheme = ColorScheme(
    brightness: Brightness.light,
    primary: accent,
    onPrimary: Color(0xFFFFFCF7),
    secondary: Color(0xFF2F6B4F),
    onSecondary: Color(0xFFFFFCF7),
    error: Color(0xFF9C2F2F),
    onError: Color(0xFFFFFCF7),
    surface: card,
    onSurface: ink,
  );
  return ThemeData(
    useMaterial3: true,
    colorScheme: scheme,
    scaffoldBackgroundColor: paper,
    appBarTheme: const AppBarTheme(
      backgroundColor: paper,
      foregroundColor: ink,
      elevation: 0,
      scrolledUnderElevation: 0,
    ),
    filledButtonTheme: FilledButtonThemeData(
      style: FilledButton.styleFrom(
        minimumSize: const Size.fromHeight(48),
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      ),
    ),
    outlinedButtonTheme: OutlinedButtonThemeData(
      style: OutlinedButton.styleFrom(
        minimumSize: const Size.fromHeight(48),
        foregroundColor: ink,
        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(12)),
      ),
    ),
  );
}
