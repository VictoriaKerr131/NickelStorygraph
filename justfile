set dotenv-load

export VERSION := "0.1.0"

[group('helpers')]
default:
  @{{ just_executable() }} --list --justfile {{ justfile() }}

# Prepare the Qt6 toolchain docker image (Qt 6.5.2 + Rust + resvg)
[group('build')]
build-tc:
  docker build .forgejo/nickeltc --build-arg UID=$(id --user) --build-arg GID=$(id --group) --tag strayrose/nickeltc

# Render modified svgs to png
[group('build')]
[working-directory('hook/res')]
build-res:
  #!/usr/bin/env sh
  for svg in *.svg; do
    png="${svg%.svg}.png"
    if [ ! -e "$png" -o "$svg" -nt "$png" ]; then
      echo "$png"
      resvg "$svg" "$png"
    fi
  done

# Build the nickel QT plugin (Qt6)
[group('build')]
[working-directory('hook')]
build-hook:
  make

# Build the rust CLI
[group('build')]
[working-directory('cli')]
build-cli:
  cargo build --release --target=arm-unknown-linux-gnueabihf

[group('build')]
build: build-res build-hook build-cli

# Package files into installable archives.
# Qt6 firmware extracts Kobo.tgz under /usr/local/Kobo; the CLI and config go
# under mnt/onboard/.adds and must be copied to the device separately.
[group('package')]
package:
  #!/usr/bin/env sh
  cd hook
  make kobo
  cd ..
  rm -rf mnt
  mkdir -p mnt/onboard/.adds/NickelStorygraph
  cp cli/target/arm-unknown-linux-gnueabihf/release/nickel-storygraph-cli mnt/onboard/.adds/NickelStorygraph/cli
  cp hook/res/config_example.ini                                          mnt/onboard/.adds/NickelStorygraph/

# Build the package within the toolchain docker container
[group('package')]
build-package:
  docker run --volume="$PWD:$PWD" --workdir="$PWD" --volume="nickeltc-cargo-cache:/cargo/registry" --rm strayrose/nickeltc just build package

# Copy the package onto a kobo device
[group('package')]
copy-package: build-package
  #!/usr/bin/env sh
  cp hook/Kobo.tgz /media/$(whoami)/KOBOeReader/.kobo/
  cp -r mnt/onboard/.adds/NickelStorygraph /media/$(whoami)/KOBOeReader/
  sudo eject /media/$(whoami)/KOBOeReader/

# Format all source files
[group('helpers')]
format:
  cd cli && cargo fmt
  clang-format -i hook/src/**/*.cc hook/src/*.cc
  clang-format -i hook/src/**/*.h hook/src/*.h

[group('helpers')]
clean:
  cd hook && make clean
  cd cli && cargo clean
  rm -fv KoboRoot.tgz hook/Kobo.tgz
  rm -rf mnt
  for f in hook/res/*.png; do git ls-files --error-unmatch $f > /dev/null 2>&1 || rm -v $f; done

# Run `logread` over ssh
[group('helpers')]
logs:
  #!/usr/bin/env expect
  spawn ssh $::env(KOBO_SERVER)
  expect "password: "
  send "$::env(KOBO_PASSWORD)\r"
  expect -ex {[root@kobo ~]#}
  send "logread -f\r"
  interact
