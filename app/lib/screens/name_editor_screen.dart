import 'dart:convert';

import 'package:flutter/material.dart';

import '../gatt.dart';
import '../models.dart';
import '../repository.dart';

class NameEditorScreen extends StatefulWidget {
  const NameEditorScreen({super.key, required this.id, required this.initialName});

  final String id;
  final String initialName;

  @override
  State<NameEditorScreen> createState() => _NameEditorScreenState();
}

class _NameEditorScreenState extends State<NameEditorScreen> {
  late final TextEditingController _controller = TextEditingController(text: widget.initialName);
  String? _error;
  bool _saving = false;

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  int get _bytes => utf8.encode(_controller.text).length;

  Future<void> _save() async {
    final name = _controller.text;
    if (utf8.encode(name).length > nameMaxBytes) return;
    setState(() {
      _saving = true;
      _error = null;
    });
    try {
      await CounterScope.of(context).writeName(widget.id, name);
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
          _error = 'The counter rejected that name.';
        });
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    final over = _bytes > nameMaxBytes;
    final muted = Theme.of(context).colorScheme.onSurface.withValues(alpha: 0.7);
    return Scaffold(
      appBar: AppBar(
        title: const Text('Name'),
        actions: [
          TextButton(
            onPressed: _saving || over ? null : _save,
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
            'A label for this counter, up to 32 bytes of UTF-8. An empty save clears it, and the list shows CC-XXXX.',
            style: TextStyle(color: muted, height: 1.35),
          ),
          const SizedBox(height: 16),
          TextField(
            controller: _controller,
            autofocus: true,
            textInputAction: TextInputAction.done,
            onChanged: (_) => setState(() => _error = null),
            onSubmitted: (_) {
              if (!_saving && !over) _save();
            },
            decoration: InputDecoration(
              labelText: 'Name',
              helperText: '$_bytes / $nameMaxBytes bytes',
              errorText: over ? 'That is longer than 32 bytes.' : _error,
            ),
          ),
        ],
      ),
    );
  }
}
