module Modifiers = Modifiers;
module Matcher = Matcher;

type key = {
  scancode: int,
  keycode: int,
  modifiers: Modifiers.t,
  text: string,
};

module type Input = {
  type payload;
  type context;

  type t;

  let addBinding: (Matcher.sequence, context => bool, payload, t) => (t, int);
  let addMapping:
    (Matcher.sequence, context => bool, list(key), t) => (t, int);

  type effects =
    | Execute(payload)
    | Unhandled(key);

  let keyDown: (~context: context, key, t) => (t, list(effects));
  let keyUp: (~context: context, key, t) => (t, list(effects));
  let flush: (~context: context, t) => (t, list(effects));

  let empty: t;
};

module Make = (Config: {
                 type payload;
                 type context;
               }) => {
  type payload = Config.payload;
  type context = Config.context;

  type effects =
    | Execute(payload)
    | Unhandled(key);

  type action =
    | Dispatch(payload)
    | Remap(list(key));

  type binding = {
    id: int,
    sequence: Matcher.sequence,
    action,
    enabled: context => bool,
  };

  type t = {
    nextId: int,
    allBindings: list(binding),
    keys: list(key),
  };

  let keyMatches = (keyMatcher, key) => {
    Matcher.(
      {
        switch (keyMatcher) {
        | Scancode(scancode, mods) =>
          key.scancode == scancode && Modifiers.equals(mods, key.modifiers)
        | Keycode(keycode, mods) =>
          key.keycode == keycode && Modifiers.equals(mods, key.modifiers)
        };
      }
    );
  };

  let applyKeyToBinding = (~context, key, binding) => {
    if (!binding.enabled(context)) {
      None
    } else {
      switch (binding.sequence) {
      | [hd, ...tail] when keyMatches(hd, key) =>
        Some({...binding, sequence: tail})
      | [] => None
      | _ => None
      };
    }
  };

  let applyKeyToBindings = (~context, key, bindings) => {
    List.filter_map(applyKeyToBinding(~context, key), bindings);
  };

  let applyKeysToBindings = (~context, keys, bindings) => {
    List.fold_left(
      (acc, curr) => {applyKeyToBindings(~context, curr, acc)},
      bindings,
      keys,
    );
  };

  let addBinding = (sequence, enabled, payload, bindings) => {
    let {nextId, allBindings, _} = bindings;
    let allBindings = [
      {id: nextId, sequence, action: Dispatch(payload), enabled},
      ...allBindings,
    ];

    let newBindings = {...bindings, allBindings, nextId: nextId + 1};
    (newBindings, nextId);
  };

  let addMapping = (sequence, enabled, keys, bindings) => {
    let {nextId, allBindings, _} = bindings;
    let allBindings = [
      {id: nextId, sequence, action: Remap(keys), enabled},
      ...allBindings,
    ];

    let newBindings = {...bindings, allBindings, nextId: nextId + 1};
    (newBindings, nextId);
  };

  let reset = (~keys=[], bindings) => {...bindings, keys};

  let getReadyBindings = (bindings) => {
    let filter = binding => binding.sequence == [];

    bindings
    |> List.filter(filter);
  };

  let flush = (~context, bindings) => {
    let allKeys = bindings.keys;

    let rec loop = (flush, revKeys, remainingKeys, effects) => {
      let candidateBindings =
        applyKeysToBindings(~context, revKeys |> List.rev, bindings.allBindings);
      let readyBindings = getReadyBindings(candidateBindings);
      let readyBindingCount = List.length(readyBindings);
      let candidateBindingCount = List.length(candidateBindings);

      let potentialBindingCount = candidateBindingCount - readyBindingCount;

      switch (List.nth_opt(readyBindings, 0)) {
      | Some(binding) =>
        if (flush || potentialBindingCount == 0) {
          switch (binding.action) {
          | Dispatch(payload) => (
              remainingKeys,
              [Execute(payload), ...effects],
            )
          | Remap(keys) =>
            loop(
              flush,
              List.append(List.rev(keys), revKeys),
              remainingKeys,
              effects,
            )
          };
        } else {
          (List.append(revKeys, remainingKeys), effects);
        }
      // Queue keys -
      | None when potentialBindingCount > 0 => (
          // We have more bindings available, so just stash our keys and quit
          List.append(revKeys, remainingKeys),
          effects,
        )
      // No candidate bindings... try removing a key and processing bindings
      | None =>
        switch (revKeys) {
        | [] =>
          // No keys left, we're done here
          (remainingKeys, effects)
        | [latestKey] =>
          // At the last key... if we got here, we couldn't find any match for this key
          ([], [Unhandled(latestKey), ...effects])
        | [latestKey, ...otherKeys] =>
          // Try a subset of keys
          loop(flush, otherKeys, [latestKey, ...remainingKeys], effects)
        }
      };
    };

    let (remainingKeys, effects) = loop(true, allKeys, [], []);

    let (remainingKeys, effects) = loop(false, remainingKeys, [], effects);

    let keys = remainingKeys;
    (reset(~keys, bindings), effects);
  };

  let keyDown = (~context, key, bindings) => {
    let originalKeys = bindings.keys;
    let keys = [key, ...bindings.keys];

    let candidateBindings =
      applyKeysToBindings(~context, keys |> List.rev, bindings.allBindings);

    let readyBindings = getReadyBindings(candidateBindings);
    let readyBindingCount = List.length(readyBindings);
    let candidateBindingCount = List.length(candidateBindings);

    let potentialBindingCount = candidateBindingCount - readyBindingCount;

    if (potentialBindingCount > 0) {
      ({...bindings, keys}, []);
    } else {
      switch (List.nth_opt(readyBindings, 0)) {
      | Some(binding) =>
        switch (binding.action) {
        | Dispatch(payload) => (reset(bindings), [Execute(payload)])
        | Remap(remappedKeys) =>
          let keys = List.append(originalKeys, remappedKeys);
          flush(~context, {...bindings, keys});
        }
      | None => flush(~context, {...bindings, keys})
      };
    };
  };

  let keyUp = (~context as _, _key, bindings) => (bindings, []);

  let empty = {nextId: 0, allBindings: [], keys: []};
};
