import 'dart:io' show Platform;

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

class PlatformInfo {
  static const _channel = MethodChannel('dev.chargecounter/platform');
  static int? _sdk;

  static bool get isAndroid => !kIsWeb && Platform.isAndroid;

  static Future<int> androidSdkInt() async {
    if (!isAndroid) return 0;
    final cached = _sdk;
    if (cached != null) return cached;
    final value = await _channel.invokeMethod<int>('sdkInt');
    return _sdk = value ?? 0;
  }

  static Future<void> openLocationSettings() {
    return _channel.invokeMethod<void>('openLocationSettings');
  }
}
