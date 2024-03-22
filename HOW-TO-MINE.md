# DePINC Mining Guide (2024 chia consensus)

DePINC is a peer-to-peer digital currency that uses the Proof of Space and Time
(PoST) consensus protocol.

## Hard fork

DePINC will switch the core algorithm of the consensus protocol from Burst to
Chia at a scheduled time.

* Hard-fork height - 860130

* Consensus protocol changes - Burst to Chia

* Pledge change

  1. The original pledge will be invalid, but the pledged DePC coin can be
     withdrawn
  2. A lock-up period is added to the pledge, and withdrawals will not be possible
     during the lock-up period.
  3. Pledge income is affected by the pledge time

**Ordinary users need to make changes**

* Update the latest node program

**Miners need to make changes**

* Update the latest wallet;

* Use Chia's official program to initialize the hard drive with plot files;

* After the fork height is reached, use the new mining client to re-bind and
  do the deposit;

## Prepare wallet

### Download or update your node program

Mining requires DePINC full node, you need to go to the official website
[https://depinc.org](https://depinc.org) and download the latest version of the
wallet program.

After downloading the node program, double-click the installation package to
install it. When the installation is completed, it is recommended to change the
main directory to DePINC, and add it to the search path.

The node program will automatically download all block data from the network.
Please make sure that the hard disk that stores the node data has at least 10GB
space left.

## Use Chia client to initialize the mining hard disk (P disk)

For details, please refer to Chia’s official website description:
[https://chia.net](https://chia.net)

## Download mining client

To download the mining client, please visit
[depinc-miner](https://github.com/DePINC/miner/releases) to download The latest
version. If the system you are using is Linux, please download the source code
and compile it by yourself. Find the build instructions from the project README.

## Write mining configuration file

After downloading the new version of the client, please extract all files to a
new folder. You will Find a file named `depinc-miner.exe` (under Linux, this file
exists in the Miner project directory `build/depinc-miner`). The executable file
is the main program of the mining client.

Before any mining work starts, You need to create a Json configuration file,
usually named `config.json`.

> This configuration file can have any name, but you need to ensure that its
> content meets the requirements. You can execute the command with parameter
> `-c othername.json` to set the name of the configuration file.

Executing the following command will initialize an empty configuration file in
the current directory. If no parameters are added, the name of the output
file is `config.json`:

```bash
depinc-miner.exe generate-config
```

Or generate the configuration file with a specified path and name:

```bash
depinc-miner.exe -c othername.json generate-config
```

You need to add some entries to tell miner client that how to do the mining.

* reward - Fill in a DePC address, which will receive the rewards once you make
  a new block to the chain;

* seed - This is the mnemonic in your Chia wallet. It generates your farmer's
  public-key, which is written in your plot files, and the mining program needs
  to use this seed to sign a new blocks.

> This mnemonic is only used for local signatures and will not be uploaded or
> used elsewhere. You can add add multiple sets of mnemonic words, miner client
> will use it based on the plot file you saved the best answer when scanning the
> hard disk and sign with the corresponding mnemonic;

* plotPath - Tell miner client where to find the plots. The type of the item is
  array and multiple folders can be added;

* rpc - Tell the miner client how to connect to your node program.

> There are three entries should be provided: `host`, `username`, `password`.
> If `username` and `password` are not provided, the mining program will find and
> read `.cookie` file to obtain  authentication information for establishing a
> connection to node.
> Use parameter `--cookie` to set the `.cookie` file when the miner client cannot
> find it.

* timelords - The program to run VDf calculator helps you obtain the VDF proof.
  Please read the following chapters for information about Timelord.
  If you don't know how to fill in this entry, please use `timelord.bhd.one:29292`
  as an option, but this is not the best choice.

A example of the config file:

```json
{
    "reward": "38CLnjuj31ifZMXZV8UhbyCo3fNP46Lszy",
    "seed": "bird convince trend skin lumber escape crater describe ...",
    "plotPath": [
        "/home/matthew/data/plotfiles1",
        "/home/matthew/data/plotfiles2"
    ],
    "rpc": {
        "host": "http://127.0.0.1:18732"
    },
    "timelords": [
        "timelord.bhd.one:29292"
    ]
}
```

## Bind to Chia’s farmer public-key

Before starting mining, you need to bind your reward address with your Chia's
farmer public-key and submit the transaction to DePINC peer-to-peer network.

You need to ensure:

* Synchronized node program;

* The configuration of `config.json` is completed and correct, and the mining
  program can connect to the node program;

* Each bind transaction needs 0.1 DePC, you need to ensure the main wallet has
  enough coins.

Run the following command to bind:

```bash
depinc-miner.exe bind
```

Transaction id will be shown to you after the command is executed successfully.
If it fails, please check the return information and try again.

> If your configuration file contains multiple mnemonics, you can refer to them
> by adding the parameter `--index`. Determine the Farmer public-key corresponding
> to the mnemonic words to be bound. Please bind all mnemonics.

For example, to bind the second mnemonic phrase, use the following command:

```bash
depinc-miner.exe bind --index=1
```

> The mnemonic sequence number starts from 0

## Deposit DePC to maximize mining profits

When you don't do any deposit, you can only get 10% of the rewards each block,
deposit with an appropriate amount to get full rewards. Check
[white paper](https://github.com/depinc/white-paper) to get more information.

### How to make a pledge?

Run the following command to do the deposit:

```bash
depinc-miner.exe deposit --amount 5 --term term3
```

> This command will stake 5 DePC to the network, and the deposit type is `term3`.

## Start mining client

Finally, you are able to run the following command to start the mining client:

```bash
depinc-miner.exe mining
```

> If your configuration file is not in the same directory where miner client is,
> you can use `-c` to specify the configuration file, otherwise the mining
> program will report error and exit.

## Run VDF to obtain time-proof data (Linux only)

### What are Timelord and VDF?

* VDF is the proof of time in the Chia consensus protocol. A VDF proof can prove
  that in order to obtain the proof, there must be the corresponding time elapses.
  The verification process cannot be accelerated through parallelism, so it
  eliminates the need for GPUs or dedicated mining machines.

* Time proof will greatly increase the security of the network. Each new block
  stores a VDF proof, which allows the chain to verify. It becomes very difficult
  to use large computing power to regenerate the entire network data.

* The results calculated by each VDF in the network will be consistent, and VDFs
  running in different places are not competitive. At least one VDF calculator
  must be running in the entire network.

* Timelord is the manager of the VDF prover, we use it to manage the time prover
  associated with the chain, and coordinate and obtain time certificates.

 ### What are the benefits of running VDF locally?

* If you do not run VDF locally, you need to wait for the VDF proof from the
  P2P network, which may cause block generation due to network delays.
  which prevents blocks from being discovered by the entire network in time;

* Running a VDF will submit the answer to the entire network, making the DePINC
  network more stable and secure;

### Conditions for running VDF locally

* A CPU with decent speed, such as the latest i7.

* Use Linux operating system to run VDF calculator and timelord;

> Note 1: Because Chia’s official VDF is only optimized for the Linux operating
> system, although it can also be used under Windows to perform VDF calculation,
> but the speed is much slower than Linux, so we do not recommend users to run
> it under Windows.

### How to start VDF calculation?

* Visit Timelord's code repository and get the latest version of the program.
  You can access this link: [Timelord](https://github.com/depinc/timelord)

* Visit DePINC’s [Wiki](https://github.com/DePINC/depinc/wiki) more instructions
  for using Timelord are available.

> Note: To solve multiple VDF proofs of the same challenge, you only need to
> start one `vdf_client`, no matter how many are received. Different VDF proof
> requests will not cause excessive computer resource consumption.

### VDF Proof Answer via P2P Propagation

When your miner client is not connected to any VDF calculation program, it will
request the VDF calculation to the P2P network. If a Timelord program receives
the request through the P2P, it will also send the relevant proofs to P2P network
when the answer is obtained.

> However, obtaining VDF answers through the P2P network may be unstable, and
> you cannot determine whether there is a Timelord machine running on the network?
