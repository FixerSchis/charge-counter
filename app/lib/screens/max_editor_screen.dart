import 'package:flutter/material.dart';

import '../gatt.dart';
import '../models.dart';
import '../repository.dart';

class MaxEditorScreen extends StatefulWidget {
  const MaxEditorScreen({super.key, required this.id, required this.initialMax});

  final String id;
  final int initialMax;

  @override
  State<MaxEditorScreen> createState() => _MaxEditorScreenState();
}

class _MaxEditorScreenState extends State<MaxEditorScreen> {
  late int _value = widget.initialMax.clamp(maxLower, maxUpper);
  String? _error;
  bool _saving = false;

  Future<void> _save() async {
    setState(() {
      _saving = true;
      _error = null;
    });
    try {
      await CounterScope.of(context).writeMax(widget.id, _value);
      if (mounted) Navigator.pop(context);
    } on CounterEditException catch (error) {
      if (mounted) {
        setState(() {
          _saving = false;
          _error = error.message;
        });
      }
    } catch (_) {
      if (mounted) {
        setState(() {
          _saving = false;
          _error = 'The counter rejected that maximum.';
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final muted = Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.7);
    return Scaffold(
      appBar: AppBar(
        title: const Text('Maximum'),
        actions: [
          TextButton(
            onPressed: _saving ? null : _save,
            child: _saving
                ? const SizedBox(width: 18, height: 18, child: CircularProgressIndicator(strokeWidth: 2))
                : const Text('Save'),
          ),
        ],
      ),
      body: ListView(
        padding: const EdgeInsets.all(20),
        children: [
          Text(
            'The highest count this counter will store. Saving writes only this counter. If the current count is higher, the counter clamps it and the detail screen shows the new count.',
            style: TextStyle(color: muted, height: 1.35),
          ),
          const SizedBox(height: 20),
          Wrap(
            spacing: 8,
            runSpacing: 8,
            children: [
              for (var value = maxLower; value <= maxUpper; value++)
                ChoiceChip(
                  label: Text('$value'),
                  selected: value == _value,
                  onSelected: _saving
                      ? null
                      : (_) => setState(() {
                            _value = value;
                            _error = null;
                          }),
                ),
            ],
          ),
          if (_error != null) ...[
            const SizedBox(height: 16),
            Text(_error!, style: TextStyle(color: Theme.of(context).colorScheme.error)),
          ],
        ],
      ),
    );
  }
}
