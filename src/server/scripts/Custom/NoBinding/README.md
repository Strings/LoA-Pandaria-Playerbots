# No binding

`NoBinding.Enable = 1` suppresses ordinary item soulbinding, including the
transmogrification write and binding enchants. Quest items, quest starters,
artifact-quality items and `NoBinding.ProtectedItemIds` remain protected.
Invalid protected IDs disable the feature safely. No timers are introduced.

The core adapter in Entities/Item/NoBinding owns the policy. Other progression
systems register an instance protection predicate with `RegisterProtectionCheck`
at startup, before player loading. This supports detached storage, dormant
items and account copies without coupling the binding policy to those systems.
The pure BindingRules header has no server dependencies.

The flag-write hook keeps Item.h unchanged. Item loading and config reload also
clear ordinary existing bindings. Disabling restores normal future binding;
it does not reconstruct bindings previously cleared by the cleanup.

Item-sparse replies expose effective binding/account flags. Hotfix metadata
versions advance on config reload, including disabling/protecting an item, so
cached tooltips can revert. The world SQL supplies the affected metadata rows.
`NoBinding.IntroOnLogin` controls the brief human login notification.

GM commands (the existing additem permission): `.nobinding status`,
`.nobinding pickup ENTRY` (normal pickup, without .additem's GM unbinding pass), `.nobinding item SLOT`, `.nobinding bind SLOT` (exercise the real binding write),
and `.nobinding refresh`. Item commands use the selected player, otherwise self;
slots are equipment/backpack/bank positions in the main inventory container.
Refresh also resends hotfix metadata. The cleanup generator and live steps are
documented with their implementation below.
