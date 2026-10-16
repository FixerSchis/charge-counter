import 'dart:async';

import 'package:flutter/material.dart';

import '../models.dart';
import '../radio_banner.dart';
import '../repository.dart';
import 'add_counter_screen.dart';
import 'device_detail_screen.dart';

class DeviceListScreen extends StatefulWidget {
  const DeviceListScreen({super.key});

  @override
  State<DeviceListScreen> createState() => _DeviceListScreenState();
}

class _DeviceListScreenState extends State<DeviceListScreen> {
  Timer? _clock;

  @override
  void initState() {
    super.initState();
    _clock = Timer.periodic(const Duration(seconds: 30), (_) {
      if (mounted) setState(() {});
    });
  }

  @override
  void dispose() {
    _clock?.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    final repository = CounterScope.of(context);
    final counters = repository.counters;
    return Scaffold(
      appBar: AppBar(title: const Text('Charge Counter')),
      body: Column(
        children: [
          RadioBanner(repository: repository),
          Expanded(
            child: counters.isEmpty
                ? const _EmptyList()
                : ListView.separated(
                    padding: const EdgeInsets.fromLTRB(20, 12, 20, 96),
                    itemCount: counters.length,
                    separatorBuilder: (_, _) => const SizedBox(height: 12),
                    itemBuilder: (context, index) {
                      final counter = counters[index];
                      return _CounterRow(
                        counter: counter,
                        onTap: () {
                          Navigator.push(
                            context,
                            MaterialPageRoute<void>(
                              builder: (_) => DeviceDetailScreen(id: counter.id),
                            ),
                          );
                        },
                      );
                    },
                  ),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: () {
          Navigator.push(
            context,
            MaterialPageRoute<void>(builder: (_) => const AddCounterScreen()),
          );
        },
        icon: const Icon(Icons.add),
        label: const Text('Add a counter'),
      ),
    );
  }
}

class _EmptyList extends StatelessWidget {
  const _EmptyList();

  @override
  Widget build(BuildContext context) {
    final muted = Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.7);
    return Padding(
      padding: const EdgeInsets.all(32),
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Text('No counters on this phone yet.', style: Theme.of(context).textTheme.titleMedium),
          const SizedBox(height: 8),
          Text(
            'Add one while its lamps are blinking blue. A counter this phone has not bonded stays off this list.',
            textAlign: TextAlign.center,
            style: TextStyle(color: muted),
          ),
        ],
      ),
    );
  }
}

class _CounterRow extends StatelessWidget {
  const _CounterRow({required this.counter, required this.onTap});

  final CachedCounter counter;
  final VoidCallback onTap;

  @override
  Widget build(BuildContext context) {
    final theme = Theme.of(context);
    final muted = theme.colorScheme.onSurface.withValues(alpha: 0.65);
    final secondary = counter.name.isNotEmpty
        ? '${counter.hardwareName} · ${_maxLabel(counter)}'
        : _maxLabel(counter);
    final battery = counter.battery == null ? 'Battery unknown' : 'Battery ${counter.battery}%';
    return Material(
      color: theme.colorScheme.surface,
      borderRadius: BorderRadius.circular(16),
      child: InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(16),
        child: Padding(
          padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 14),
          child: Row(
            children: [
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(counter.label, style: theme.textTheme.titleMedium),
                    const SizedBox(height: 4),
                    Text(secondary, style: TextStyle(color: muted)),
                    const SizedBox(height: 4),
                    Text('$battery · ${formatPresence(counter)}', style: TextStyle(color: muted)),
                  ],
                ),
              ),
              const SizedBox(width: 12),
              Text(
                counter.count?.toString() ?? '—',
                style: theme.textTheme.headlineMedium?.copyWith(fontFeatures: const [FontFeature.tabularFigures()]),
              ),
            ],
          ),
        ),
      ),
    );
  }

  static String _maxLabel(CachedCounter counter) {
    return counter.max == null ? 'max —' : 'max ${counter.max}';
  }
}
