# CN8CardSaver for NVIDIA GPUs

CN8CardSaver is a high performance Monero (XMR) NVIDIA miner forked from XMRig-nvidia.

cn8cardsaver (CryptoNight V1/2) is a miner for Monero XMR with GPU temperature control support. With it you can keep your expensive cards save.
Keep it below 65 C to be on the safe side. If it gets to 80 C or above you are damaging your card.
Use the options ```--max-gpu-temp=65 and --gpu-temp-falloff=9```

GPU mining part based on [psychocrypt](https://github.com/psychocrypt) code used in xmr-stak-nvidia.

Temperature control:
### Command line options
```
      --max-gpu-temp=N      Maximum temperature a GPU may reach before its cooled down (default 75)
      --gpu-temp-falloff=N  Amount of temperature to cool off before mining starts again (default 10)
      --gpu-fan-level=N     -1 disabled | 0 automatic (default) | 1..100 Fan speed in percent
```

#### Supported algorithms
* CryptoNight (Monero)
* CryptoNight-Lite
* CryptoNight-Heavy
* Original CryptoNight or CryptoNight-Heavy
* CryptoNight variant 1 also known as Monero7 and CryptoNightV7
* Modified CryptoNight-Heavy (TUBE only)
* Modified CryptoNight variant 1 (Stellite only)
* Modified CryptoNight variant 1 (Masari only)
* Modified CryptoNight-Heavy (Haven Protocol only)
* Modified CryptoNight variant 0 (Alloy only)
* Modified CryptoNight variant 1 (Arto only)
* CryptoNight variant 2

#### Table of contents
* [Features](#features)
* [Download](#download)
* [Usage](#usage)
* [Donations](#donations)
* [Contacts](#contacts)

## Features
* High performance.
* Official Windows support.
* Support for backup (failover) mining server.
* CryptoNight-Lite support for AEON.
* Automatic GPU configuration.
* GPU health monitoring (clocks, power, temperature, fan speed) 
* GPU temperature management (option --max-gpu-temp, --gpu-temp-falloff)
* Nicehash support.
* It's open source software.

## Download
* Binary releases: https://github.com/kimxilxyong/cn8cardsaver-nvidia/releases
* Git tree: https://github.com/kimxilxyong/cn8cardsaver-nvidia.git
* Clone with `git clone https://github.com/kimxilxyong/cn8cardsaver-nvidia.git`

## Usage
Use [config.xmrig.com](https://config.xmrig.com/nvidia) to generate, edit or share configurations.

### Command line options
```
  -a, --algo=ALGO          specify the algorithm to use
                             cryptonight
                             cryptonight-lite
                             cryptonight-heavy
  -o, --url=URL             URL of mining server
  -O, --userpass=U:P        username:password pair for mining server
  -u, --user=USERNAME       username for mining server
  -p, --pass=PASSWORD       password for mining server
      --rig-id=ID           rig identifier for pool-side statistics (needs pool support)
  -k, --keepalive           send keepalived packet for prevent timeout (needs pool support)
      --nicehash            enable nicehash.com support
      --tls                 enable SSL/TLS support (needs pool support)
      --tls-fingerprint=F   pool TLS certificate fingerprint, if set enable strict certificate pinning
  -r, --retries=N           number of times to retry before switch to backup server (default: 5)
  -R, --retry-pause=N       time to pause between retries (default: 5)
      --cuda-devices=N      list of CUDA devices to use.
      --cuda-launch=TxB     list of launch config for the CryptoNight kernel
      --cuda-max-threads=N  limit maximum count of GPU threads in automatic mode
      --cuda-bfactor=[0-12] run CryptoNight core kernel in smaller pieces
      --cuda-bsleep=N       insert a delay of N microseconds between kernel launches
      --cuda-affinity=N     affine GPU threads to a CPU
      --max-gpu-temp=N      Maximum temperature a GPU may reach before its cooled down (default 75)
      --gpu-temp-falloff=N  Amount of temperature to cool off before mining starts again (default 10)
      --gpu-fan-level=N     -1 disabled| 0 automatic (default) | 1..100 Fan speed in percent|
      --no-color            disable colored output
      --variant             algorithm PoW variant
      --donate-level=N      donate level, default 5% (5 minutes in 100 minutes)
      --user-agent          set custom user-agent string for pool
  -B, --background          run the miner in the background
  -c, --config=FILE         load a JSON-format configuration file
  -l, --log-file=FILE       log all output to a file
  -S, --syslog              use system log for output messages
      --print-time=N        print hashrate report every N seconds
      --api-port=N          port for the miner API
      --api-access-token=T  access token for API
      --api-worker-id=ID    custom worker-id for API
      --api-id=ID           custom instance ID for API
      --api-ipv6            enable IPv6 support for API
      --api-no-restricted   enable full remote access (only if API token set)
      --dry-run             test configuration and exit
  -h, --help                display this help and exit
  -V, --version             output version information and exit
```

## Donations
Default donation 5% (5 minutes in 100 minutes) can be reduced to 1% via option `donate-level`.

* XMR: `422KmQPiuCE7GdaAuvGxyYScin46HgBWMQo4qcRpcY88855aeJrNYWd3ZqE4BKwjhA2BJwQY7T2p6CUmvwvabs8vQqZAzLN`
* BTC: `19hNKKFu34CniRWPhGqAB76vi3U4x7DZyZ`

#### Donate to xmrig dev
* XMR: `48edfHu7V9Z84YzzMa6fUueoELZ9ZRXq9VetWzYGzKt52XU5xvqgzYnDK9URnRoJMk1j8nLwEVsaSWJ4fhdUyZijBGUicoD`
* BTC: `1P7ujsXeX7GxQwHNnJsRMgAdNkFZmNVqJT`

## Contacts
* kimxilxyong@gmail.com
* [reddit](https://www.reddit.com/user/kimilyong/)

