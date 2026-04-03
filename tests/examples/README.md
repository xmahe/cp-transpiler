# Examples

This folder holds larger sample `c+` inputs that are meant to look closer to a real embedded module than the small smoke tests.

Paired-module rule:

- a sample module is usually split across a matching `.hp` and `.cp`
- both files share the same stem and describe one logical unit
- header declarations live in `.hp`, while definitions and executable bodies live in `.cp`

The current example is intentionally declaration-heavy because the transpiler is still growing body lowering. It still exercises the important language pieces:

- namespace blocks
- enums
- interfaces
- classes
- `maybe<T>`
- free functions

Use it as a reference shape for how a real `c+` module is expected to look.
