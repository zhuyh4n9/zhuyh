# A Stackful Coroutine Library
## Build
  - clean
  ```sh
    sh build.sh clean
  ```
  - build
  ```
    sh build.sh build
  ```
  - clean & build
  ```
    sh build.sh force_build
  ```
## requirement
  - boost library is required
## Future Plan
  - support io_uring & aio
  - add support for hash & crypto
  - support C++20 (optional)
  - support variaous schedule algorithm(CFQ/DEADLINE ...)
  - support channel for communication between coroutine

```
