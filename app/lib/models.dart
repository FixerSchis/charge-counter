/// One bonded counter, keyed by the stable BLE address for this phone.
class CachedCounter {
  CachedCounter({
    required this.id,
    this.name = '',
    this.advName = '',
    this.count,
    this.max,
    this.battery,
    this.lastSeen,
  });

  final String id;
  String name;
  String advName;
  int? count;
  int? max;
  int? battery;
  DateTime? lastSeen;
  bool connected = false;
  bool inRange = false;

  /// Advertised `CC-XXXX`, or the same form derived from an Android address.
  String get hardwareName {
    if (advName.isNotEmpty) return advName;
    return derivedHardwareName(id);
  }

  /// Stored name, or `CC-XXXX` when the device label is empty.
  String get label => name.isNotEmpty ? name : hardwareName;

  Map<String, dynamic> toJson() => {
        'id': id,
        'name': name,
        'advName': advName,
        'count': count,
        'max': max,
        'battery': battery,
        'lastSeen': lastSeen?.millisecondsSinceEpoch,
      };

  static CachedCounter? fromJson(Map<String, dynamic> json) {
    final id = json['id'];
    if (id is! String || id.isEmpty) return null;
    return CachedCounter(
      id: id,
      name: json['name'] is String ? json['name'] as String : '',
      advName: json['advName'] is String ? json['advName'] as String : '',
      count: _asInt(json['count']),
      max: _asInt(json['max']),
      battery: _asInt(json['battery']),
      lastSeen: _asTime(json['lastSeen']),
    );
  }
}

class PairingOffer {
  const PairingOffer({required this.id, required this.advName});

  final String id;
  final String advName;

  String get label => advName.isNotEmpty ? advName : derivedHardwareName(id);
}

class CounterEditException implements Exception {
  const CounterEditException(this.message);
  final String message;

  @override
  String toString() => message;
}

/// Lower 16 bits of an Android MAC, matching the firmware's `CC-XXXX`.
String derivedHardwareName(String id) {
  final parts = id.split(':');
  if (parts.length == 6 && parts.every((part) => part.length == 2)) {
    return 'CC-${(parts[4] + parts[5]).toUpperCase()}';
  }
  return 'CC-????';
}

String formatWhen(DateTime? time) {
  if (time == null) return 'not seen yet';
  final delta = DateTime.now().difference(time);
  if (delta.isNegative || delta.inSeconds < 45) return 'just now';
  if (delta.inMinutes < 60) {
    final minutes = delta.inMinutes;
    return minutes == 1 ? '1 min ago' : '$minutes min ago';
  }
  if (delta.inHours < 24) {
    final hours = delta.inHours;
    return hours == 1 ? '1 hour ago' : '$hours hours ago';
  }
  if (delta.inDays == 1) return 'yesterday';
  const months = [
    'Jan',
    'Feb',
    'Mar',
    'Apr',
    'May',
    'Jun',
    'Jul',
    'Aug',
    'Sep',
    'Oct',
    'Nov',
    'Dec',
  ];
  return '${time.day} ${months[time.month - 1]}';
}

String formatPresence(CachedCounter counter) {
  final when = formatWhen(counter.lastSeen);
  if (counter.connected) return 'Connected · $when';
  if (counter.inRange) return 'Nearby · $when';
  return 'Out of range · $when';
}

int? _asInt(Object? value) => value is int ? value : null;

DateTime? _asTime(Object? value) {
  if (value is! int) return null;
  return DateTime.fromMillisecondsSinceEpoch(value);
}
