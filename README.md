# ZMK-NUM-WORD

Implement [urob's](https://github.com/urob) num-word behavior for [ZMK](https://github.com/zmkfirmware/zmk).

This is proof of concept that shows that without maintaining fork new behavior can
be implemented.

## How to use this module?

Under `config/west.yml` add `remotes` and `projects`, here is an example of
full file.

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: dhruvinsh
      url-base: https://github.com/dhruvinsh
  projects:
    - name: zmk
      remote: zmkfirmware
      revision: main
      import: app/west.yml
    - name: zmk-num-word
      remote: dhruvinsh
      revision: main
  self:
    path: config
```

**NOTE: [common use-case](https://zmk.dev/docs/development/new-behavior#defining-common-use-cases-for-the-behavior-dtsi-optional) is define.**

## Summary

The num word behavior behaves similar to a cap word, but will automatically
deactivate when any key not in a continue list is pressed, or if the num word
key is pressed again. For smaller keyboards using mod-taps, this can help avoid
repeated alternating holds when typing numbers.

The modifiers are applied only to to the number (`0` to `9`) keycodes.

### Behavior Binding

- Reference: `&num_word`

Example:

```devicetree
&num_word
```

**NOTE: Make sure you have custom behavior called `num_word` create, see above guide.**

### Configuration

Default behavior (need to assign layer) defined as,

```devicetree
/ {
    behaviors {
      num_word: num_word {
        #binding-cells = <0>;
        compatible = "zmk,behavior-num-word";
        continue-list = <BSPC DEL DOT COMMA PLUS MINUS STAR FSLH EQUAL>;
        ignore-numbers;
    };
  };
};

```

To use default behavior (do not forget to use `include`),

```devicetree
#include <behaviors/num_word.dtsi>

&num_word {
  layers = <1 2 3>;
}
```

To create new behavior,

```devicetree
#include <dt-bindings/zmk/keys.h>

/ {
    behaviors {
      new_num_word: new_num_word {
        #binding-cells = <0>;
        compatible = "zmk,behavior-num-word";
        layers = <1 2 3>;
        continue-list = <BSPC EQUAL>;
        ignore-numbers;
    };
  };
};
```

Once this define, `&new_num_word` can be use in keymap.

#### Continue List

By default, the num word will remain active when any numeric character or
`BSPC, DEL, DOT, COMMA, PLUS, MINUS, STAR, FSLH, EQUAL` characters are pressed
(see above definition). Any other non-modifier keycode sent will turn off
num word. If you would like to override this, you can set a new array of keys in
the `continue-list` property in your keymap:

```devicetree
&num_word {
    continue-list = <UNDERSCORE MINUS>;
};

/ {
    keymap {
        ...
    };
};
```

## Credits

- [Pete Johanson](https://github.com/petejohanson)
- [Robert U](https://github.com/urob)
- ZMK and Zephyr
