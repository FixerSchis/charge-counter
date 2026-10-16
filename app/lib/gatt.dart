import 'dart:convert';

import 'package:flutter_blue_plus/flutter_blue_plus.dart';

/// GATT contract from docs/ble.md and firmware/src/config.h.
class ChargeUuids {
  static final Guid service = Guid('8addf38b-7eab-4cec-a983-6597a8cebcc0');
  static final Guid count = Guid('3f037c73-e2c7-4a6f-ae69-e30ad2bc3c05');
  static final Guid max = Guid('ddc46a1f-005c-43b2-bbbe-328a76ce631b');
  static final Guid name = Guid('f4af48c7-fe47-4658-b580-155a69a8fcea');
  static final Guid locate = Guid('64da8c3f-04ac-4f61-8fc3-5a089c800558');
  static final Guid batteryService = Guid('180F');
  static final Guid batteryLevel = Guid('2A19');
}

const int nameMaxBytes = 32;
const int maxLower = 1;
const int maxUpper = 16;

int? decodeUint16(List<int> bytes) {
  if (bytes.length < 2) return null;
  return bytes[0] | (bytes[1] << 8);
}

List<int> encodeUint16(int value) => [value & 0xff, (value >> 8) & 0xff];

String decodeName(List<int> bytes) {
  var end = bytes.length;
  while (end > 0 && bytes[end - 1] == 0) {
    end--;
  }
  if (end == 0) return '';
  return utf8.decode(bytes.sublist(0, end));
}

String? tryDecodeName(List<int> bytes) {
  try {
    return decodeName(bytes);
  } on FormatException {
    return null;
  }
}

/// Pairing window: scan response service data is a single byte 0x01.
bool advertisementIsPairing(AdvertisementData data) {
  for (final entry in data.serviceData.entries) {
    final payload = entry.value;
    if (payload.isEmpty || payload.last != 0x01) continue;
    if (payload.length == 1) return true;
    if (entry.key == ChargeUuids.service) return true;
  }
  return false;
}
