import 'package:flutter/material.dart';

import '../models.dart';
import '../radio_banner.dart';
import '../repository.dart';

class AddCounterScreen extends StatefulWidget {
  const AddCounterScreen({super.key});

  @override
  State<AddCounterScreen> createState() => _AddCounterScreenState();
}

class _AddCounterScreenState extends State<AddCounterScreen> {
  String? _bondingId;
  String? _error;

  Future<void> _bond(CounterRepository repository, String id) async {
    setState(() {
      _bondingId = id;
      _error = null;
    });
    try {
      await repository.bond(id);
      if (!mounted) return;
      final added = repository.find(id);
      setState(() => _bondingId = null);
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Added ${added?.label ?? 'counter'}.')),
      );
    } on CounterEditException catch (error) {
      if (!mounted) return;
      setState(() {
        _bondingId = null;
        _error = error.message;
      });
    } catch (_) {
      if (!mounted) return;
      setState(() {
        _bondingId = null;
        _error = 'Could not add that counter.';
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    final repository = CounterScope.of(context);
    final offers = repository.pairingOffers;
    final muted = Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.7);
    return Scaffold(
      appBar: AppBar(title: const Text('Add a counter')),
      body: Column(
        children: [
          RadioBanner(repository: repository),
          Padding(
            padding: const EdgeInsets.fromLTRB(20, 16, 20, 8),
            child: Text(
              'Hold the button until the count reaches the maximum, then keep holding for three seconds. All eight lamps blink blue while this counter can be added. A counter that is nearby but not blinking is not listed. Leaving this screen keeps counters you have already added.',
              style: TextStyle(color: muted, height: 1.35),
            ),
          ),
          if (_error != null)
            Padding(
              padding: const EdgeInsets.fromLTRB(20, 8, 20, 0),
              child: Text(_error!, style: TextStyle(color: Theme.of(context).colorScheme.error)),
            ),
          Expanded(
            child: offers.isEmpty
                ? Center(
                    child: Padding(
                      padding: const EdgeInsets.all(32),
                      child: Text(
                        'Waiting for a counter in pairing mode.',
                        textAlign: TextAlign.center,
                        style: TextStyle(color: muted),
                      ),
                    ),
                  )
                : ListView.separated(
                    padding: const EdgeInsets.fromLTRB(20, 12, 20, 24),
                    itemCount: offers.length,
                    separatorBuilder: (_, _) => const SizedBox(height: 12),
                    itemBuilder: (context, index) {
                      final offer = offers[index];
                      final busy = _bondingId != null;
                      return Material(
                        color: Theme.of(context).colorScheme.surface,
                        borderRadius: BorderRadius.circular(16),
                        child: ListTile(
                          title: Text(offer.label),
                          subtitle: const Text('Pairing'),
                          enabled: !busy,
                          trailing: _bondingId == offer.id
                              ? const SizedBox(
                                  width: 24,
                                  height: 24,
                                  child: CircularProgressIndicator(strokeWidth: 2),
                                )
                              : const Icon(Icons.link),
                          onTap: busy ? null : () => _bond(repository, offer.id),
                        ),
                      );
                    },
                  ),
          ),
        ],
      ),
    );
  }
}
