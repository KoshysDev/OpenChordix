# Changelog

## [0.4.0](https://github.com/KoshysDev/OpenChordix/compare/v0.3.0...v0.4.0) (2026-01-05)


### Features

* **devtools:** add debug console ([34db9c5](https://github.com/KoshysDev/OpenChordix/commit/34db9c5829ba7a95c5a914bff01ac3b7ddf2e73c))
* **devtools:** add debug console with command registry and default commands ([32da875](https://github.com/KoshysDev/OpenChordix/commit/32da87595f5e0cbc88c039bab6eb2dafa123d2e8))


### Bug Fixes

* **ui:** feed ImGui keyboard/char input from GLFW ([adce69f](https://github.com/KoshysDev/OpenChordix/commit/adce69f0bf1e13dc2ec1c740306d6a804c34900c))

## [0.3.0](https://github.com/KoshysDev/OpenChordix/compare/v0.2.0...v0.3.0) (2026-01-03)


### Features

* add  png banner splash screen on launch ([0f48f08](https://github.com/KoshysDev/OpenChordix/commit/0f48f084fc438e1f5a26681821c8a0399dd1bfd7))
* add desktop icon ([b3b6644](https://github.com/KoshysDev/OpenChordix/commit/b3b6644e7f8592aa7d02ab1955ef04210598aa92))
* add main menu UI with placeholder actions ([c426991](https://github.com/KoshysDev/OpenChordix/commit/c4269913e59400f4d5a916767e3bd467e34422ac))
* add selectable audio output routing in setup scene ([6c636b7](https://github.com/KoshysDev/OpenChordix/commit/6c636b7c58a024058ed7577987dab9a80e787c5f))
* add settings scene ([116e662](https://github.com/KoshysDev/OpenChordix/commit/116e6628b48b92e9b04db87b4c699d5f51b5ba06))
* add tuner scene and smooth input ([5e24223](https://github.com/KoshysDev/OpenChordix/commit/5e24223fe32025bcd850c201b6414b2e32f7918e))
* **app,core:** add track selection, catalog services, and audio refactor ([381e339](https://github.com/KoshysDev/OpenChordix/commit/381e339feeb8ebe6852a43bcdb8d8a6554da1146))
* **app:** add scene based startup flow with audio session, graphics helpers and ui ([806d2e7](https://github.com/KoshysDev/OpenChordix/commit/806d2e72c16447c22548ffc246fca1edf634c741))
* **app:** add scene based startup flow with audio session, graphics helpers and ui ([082d3e1](https://github.com/KoshysDev/OpenChordix/commit/082d3e12f8568f9a9630b8933fea7cc2d9dcdb11))
* **app:** add track select scene ([8022104](https://github.com/KoshysDev/OpenChordix/commit/802210465c175b17b72b09b271ef5cccc564945d))
* **audio:** implement pitch detection using Aubio ([24bf70c](https://github.com/KoshysDev/OpenChordix/commit/24bf70cbb6588da103caa50e27135a877c0d3a70))
* **audio:** Implemented note detection mechanism ([706ce41](https://github.com/KoshysDev/OpenChordix/commit/706ce4158aab85fea2f6e4bb97b4b0fa1662b558))
* **ci:** remove GitHub Actions workflow ([69cf335](https://github.com/KoshysDev/OpenChordix/commit/69cf3351991b35a052e817a0dedd44b89c2491b8))
* **config:** persist audio setup and skip setup when valid config found ([e1018e7](https://github.com/KoshysDev/OpenChordix/commit/e1018e7727f00f5a2d6c679ccf7a0cb107438a74))
* **core:** add track catalog and score services ([94174d7](https://github.com/KoshysDev/OpenChordix/commit/94174d73de16526061576a6ebc3b87c6a6cb3e28))
* **render:** add bgfx renderer and wayland window handling ([5b869d7](https://github.com/KoshysDev/OpenChordix/commit/5b869d724055e13764f376f39086393ef74ec82d))


### Bug Fixes

* **build:** Add diagnostic part, fully linking folder for aubio lib ([139dd5f](https://github.com/KoshysDev/OpenChordix/commit/139dd5f53930df747084037d9efeecec66c8060a))
* **build:** Link aubio library name when CMake target is missing ([5faf532](https://github.com/KoshysDev/OpenChordix/commit/5faf5321bcae49b239de0aa36962ea0f3440f53a))
* **build:** Use pkg-config for dependencies on Arch Linux ([765bfb4](https://github.com/KoshysDev/OpenChordix/commit/765bfb4c1440145345ae92ea31b2ef372467cf88))
* **ci:** Correct workflow shell logic and cache key context ([8c203a6](https://github.com/KoshysDev/OpenChordix/commit/8c203a6e92831f358a5bcfd91248c3fef85001a1))
* **ci:** Disable aubio default features to workaround ffmpeg find issue ([1332bb2](https://github.com/KoshysDev/OpenChordix/commit/1332bb2f27b6ead69e64a95ff80e9d5c0fd40e71))
* **ci:** install pkg-config on Windows and aubio on Arch for successful builds ([4381311](https://github.com/KoshysDev/OpenChordix/commit/4381311576d71b0b9d976279132e05596e657411))
* **ci:** Removed 'submodules: true' ([dd4e386](https://github.com/KoshysDev/OpenChordix/commit/dd4e386cb5da877ca1a9a7656990b98092d9a755))
* **ci:** Removed metrics, and vcpkg force install ([0749f5a](https://github.com/KoshysDev/OpenChordix/commit/0749f5a91970c35cd75e089cc3705dca41fa36b3))
* **ci:** trying to directly point to psapi according to issue vcpkg [#23021](https://github.com/KoshysDev/OpenChordix/issues/23021) ([c514d07](https://github.com/KoshysDev/OpenChordix/commit/c514d07632cf8d36cf4b81b6228cfc431569734c))
* **cmake:** Add fallback for aubio linking when target is missing ([53acbea](https://github.com/KoshysDev/OpenChordix/commit/53acbea9855d1524ba43fddfee6e11029dcc05a5))
* **cmake:** correctly link aubio and rtaudio on Windows via vcpkg targets ([3f96061](https://github.com/KoshysDev/OpenChordix/commit/3f960616a05ff71a5d7c00686180844917af3694))
* **cmake:** fix typo on line 115 ([88a4bc4](https://github.com/KoshysDev/OpenChordix/commit/88a4bc49bd18a00c6c6ba8a2b780e2ae646745d1))
* **cmake:** handle platform-specific dependencies for RtAudio and Aubio ([5f8dcb7](https://github.com/KoshysDev/OpenChordix/commit/5f8dcb7551eda6c0748a190e9226e35589d1d3dd))
* **cmake:** Use correct debug/optimized names for aubio static lib linking ([2591f59](https://github.com/KoshysDev/OpenChordix/commit/2591f596689a1c3ba38b30ba2f0fee26820b94bc))
* **cmake:** Use debug/optimized keywords for aubio linking fallback ([7e32bcb](https://github.com/KoshysDev/OpenChordix/commit/7e32bcbe2b3e5e930922ce6bd402ca6a8c4e1c57))
* **github-actions:** move upload to the end of the chain ([0a350a4](https://github.com/KoshysDev/OpenChordix/commit/0a350a44c9462c6a2a381f0e1266d77fe9f7ca8e))
* **graphics:** prefer X11 on Wayland when Wayland handles are disabled ([36b8232](https://github.com/KoshysDev/OpenChordix/commit/36b8232e5398da32525fe50aa5a3496fcfe36f1d))
* prevent tracking build directory ([8da17e9](https://github.com/KoshysDev/OpenChordix/commit/8da17e9e2c3b1f8a7fe29ed88992b1fa811c712c))
* **readme:** removed wrong symbols ([2007bfd](https://github.com/KoshysDev/OpenChordix/commit/2007bfd45fbd3391c0ceaeaf2e3c1af40712dc95))
* **vcpkg:** Applying posible fix for ffmpeg lib error from:dg0yt [#45180](https://github.com/KoshysDev/OpenChordix/issues/45180) commit ([cea7dc3](https://github.com/KoshysDev/OpenChordix/commit/cea7dc340759c5f6bdc3ea0b17dab6c4f97b5016))
* **vcpkg:** Update builtin-baseline ([34618a7](https://github.com/KoshysDev/OpenChordix/commit/34618a70b94b398f6eee04f5e50e0b5b07031b44))
