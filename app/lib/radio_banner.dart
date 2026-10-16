import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

import 'platform_info.dart';
import 'repository.dart';

class RadioBanner extends StatelessWidget {
  const RadioBanner({super.key, required this.repository});

  final CounterRepository repository;

  @override
  Widget build(BuildContext context) {
    final message = _message();
    if (message == null) return const SizedBox.shrink();
    final action = _action();
    return Padding(
      padding: const EdgeInsets.fromLTRB(20, 12, 20, 0),
      child: Material(
        color: const Color(0xFFF3E4D4),
        borderRadius: BorderRadius.circular(12),
        child: Padding(
          padding: const EdgeInsets.fromLTRB(16, 12, 12, 12),
          child: Row(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              const Padding(
                padding: EdgeInsets.only(top: 2),
                child: Icon(Icons.bluetooth_disabled, size: 20),
              ),
              const SizedBox(width: 12),
              Expanded(child: Text(message)),
              if (action != null) ...[
                const SizedBox(width: 8),
                TextButton(onPressed: action.onPressed, child: Text(action.label)),
              ],
            ],
          ),
        ),
      ),
    );
  }

  String? _message() {
    if (repository.locationServicesOff) {
      return 'Android needs location services turned on before it will scan for Bluetooth devices. Charge Counter does not record where you are.';
    }
    if (repository.permissionBlocked) {
      return 'Bluetooth permission is off, so counters cannot be found or read.';
    }
    if (repository.scanError != null) return repository.scanError;
    switch (repository.adapterState) {
      case BluetoothAdapterState.off:
        return 'Bluetooth is off.';
      case BluetoothAdapterState.unauthorized:
        return 'Bluetooth access is off for Charge Counter.';
      case BluetoothAdapterState.unavailable:
        return 'This phone has no Bluetooth radio.';
      case BluetoothAdapterState.on:
      case BluetoothAdapterState.unknown:
      case BluetoothAdapterState.turningOn:
      case BluetoothAdapterState.turningOff:
        return null;
    }
  }

  _BannerAction? _action() {
    if (repository.locationServicesOff) {
      return _BannerAction('Settings', () {
        unawaited(PlatformInfo.openLocationSettings().then((_) => repository.retryScan()));
      });
    }
    if (repository.permissionBlocked || repository.adapterState == BluetoothAdapterState.unauthorized) {
      return _BannerAction('Settings', () {
        unawaited(openAppSettings());
      });
    }
    if (repository.scanError != null) {
      return _BannerAction('Retry', () {
        unawaited(repository.retryScan());
      });
    }
    if (repository.adapterState == BluetoothAdapterState.off && PlatformInfo.isAndroid) {
      return _BannerAction('Turn on', () {
        unawaited(FlutterBluePlus.turnOn());
      });
    }
    return null;
  }
}

class _BannerAction {
  const _BannerAction(this.label, this.onPressed);
  final String label;
  final VoidCallback onPressed;
}
