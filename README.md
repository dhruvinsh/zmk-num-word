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

## Credits

- [Pete Johanson](https://github.com/petejohanson)
- [Robert U](https://github.com/urob)
- ZMK and Zephyr
