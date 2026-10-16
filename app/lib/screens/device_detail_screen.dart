import 'dart:async';

import 'package:flutter/material.dart';

import '../models.dart';
import '../repository.dart';
import 'max_editor_screen.dart';
import 'name_editor_screen.dart';

class DeviceDetailScreen extends StatefulWidget {
  const DeviceDetailScreen({super.key, required this.id});

  final String id;

  @override
  State<DeviceDetailScreen> createState() => _DeviceDetailScreenState();
}

class _DeviceDetailScreenState extends State<DeviceDetailScreen> {
  String? _actionError;
  bool _acting = false;
  bool _opened = false;

  @override
  void didChangeDependencies() {
    super.didChangeDependencies();
    if (_opened) return;
    _opened = true;
    final repository = CounterScope.of(context);
    unawaited(repository.ensureConnected(widget.id, report: true).catchError((Object _) {}));
  }

  Future<void> _run(Future<void> Function() action) async {
    setState(() {
      _acting = true;
      _actionError = null;
    });
    try {
      await action();
    } on CounterEditException catch (error) {
      if (mounted) setState(() => _actionError = error.message);
    } catch (_) {
      if (mounted) setState(() => _actionError = 'That did not finish.');
    } finally {
      if (mounted) setState(() => _acting = false);
    }
  }

  Future<void> _remove(CounterRepository repository, String label) async {
    final systemBond = repository.removesSystemBond;
    final confirmed = await showDialog<bool>(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Remove this counter?'),
        content: Text(
          systemBond
              ? 'This phone drops its bond with $label, then the row. The count stays on the counter. Pair again to add it back.'
              : 'This phone drops $label from the list. The count stays on the counter. iOS also keeps a system pairing until you forget the accessory in Settings → Bluetooth, or the next pairing can fail.',
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context, false), child: const Text('Cancel')),
          TextButton(onPressed: () => Navigator.pop(context, true), child: const Text('Remove')),
        ],
      ),
    );
    if (confirmed != true || !mounted) return;
    try {
      await repository.forget(widget.id);
      if (mounted) Navigator.pop(context);
    } on CounterEditException catch (error) {
      if (mounted) setState(() => _actionError = error.message);
    } catch (_) {
      if (mounted) {
        setState(() => _actionError = 'Could not remove the bond.');
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final repository = CounterScope.of(context);
    final counter = repository.find(widget.id);
    if (counter == null) {
      return const Scaffold(body: SizedBox.shrink());
    }
    final theme = Theme.of(context);
    final muted = theme.colorScheme.onSurface.withValues(alpha: 0.7);
    final connectError = repository.connectErrorFor(widget.id);
    final busy = _acting || repository.isBusy(widget.id);
    return Scaffold(
      appBar: AppBar(title: Text(counter.label)),
      body: ListView(
        padding: const EdgeInsets.fromLTRB(20, 12, 20, 32),
        children: [
          Text(
            counter.count?.toString() ?? '—',
            textAlign: TextAlign.center,
            style: theme.textTheme.displayLarge?.copyWith(
              fontSize: 96,
              height: 1,
              fontFeatures: const [FontFeature.tabularFigures()],
            ),
          ),
          const SizedBox(height: 8),
          Text(
            counter.max == null ? 'Maximum unknown' : 'Maximum ${counter.max}',
            textAlign: TextAlign.center,
            style: theme.textTheme.titleMedium?.copyWith(color: muted),
          ),
          const SizedBox(height: 28),
          _Fact(label: 'Name', value: counter.name.isEmpty ? counter.hardwareName : counter.name),
          _Fact(label: 'Battery', value: counter.battery == null ? 'Unknown' : '${counter.battery}%'),
          _Fact(label: 'Link', value: formatPresence(counter)),
          if (connectError != null) ...[
            const SizedBox(height: 12),
            Text(connectError, style: TextStyle(color: theme.colorScheme.error)),
          ],
          if (_actionError != null) ...[
            const SizedBox(height: 12),
            Text(_actionError!, style: TextStyle(color: theme.colorScheme.error)),
          ],
          const SizedBox(height: 24),
          FilledButton(
            onPressed: busy
                ? null
                : () => _run(() async {
                      await repository.locate(widget.id);
                      if (context.mounted) {
                        ScaffoldMessenger.of(context).showSnackBar(
                          const SnackBar(content: Text('The lamps are chasing blue.')),
                        );
                      }
                    }),
            child: const Text('Locate'),
          ),
          const SizedBox(height: 12),
          OutlinedButton(
            onPressed: busy
                ? null
                : () {
                    Navigator.push(
                      context,
                      MaterialPageRoute<void>(
                        builder: (_) => NameEditorScreen(id: widget.id, initialName: counter.name),
                      ),
                    );
                  },
            child: const Text('Edit name'),
          ),
          const SizedBox(height: 12),
          OutlinedButton(
            onPressed: busy
                ? null
                : () {
                    Navigator.push(
                      context,
                      MaterialPageRoute<void>(
                        builder: (_) => MaxEditorScreen(id: widget.id, initialMax: counter.max ?? 2),
                      ),
                    );
                  },
            child: const Text('Edit maximum'),
          ),
          if (!counter.connected) ...[
            const SizedBox(height: 12),
            OutlinedButton(
              onPressed: busy
                  ? null
                  : () {
                      unawaited(
                        repository.ensureConnected(widget.id, report: true).catchError((Object _) {}),
                      );
                    },
              child: const Text('Connect'),
            ),
          ],
          const SizedBox(height: 12),
          TextButton(
            onPressed: busy ? null : () => _remove(repository, counter.label),
            child: const Text('Remove from this phone'),
          ),
        ],
      ),
    );
  }
}

class _Fact extends StatelessWidget {
  const _Fact({required this.label, required this.value});

  final String label;
  final String value;

  @override
  Widget build(BuildContext context) {
    final muted = Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.65);
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8),
      child: Row(
        children: [
          SizedBox(width: 88, child: Text(label, style: TextStyle(color: muted))),
          Expanded(child: Text(value)),
        ],
      ),
    );
  }
}
