# DePINC 挖矿指南(2024 chia consensus)

DePINC 是一款使用时空证明(Proof of Space and Time - PoST)共识协议的点对点数字货
币。

## 硬分叉

DePINC将会在既定时间将共识协议的核心算法由 Burst 切换至 Chia。

* 分叉高度 - 860130

* 节点版本2.x

* 共识协议变更 - Burst to Chia

* 质押变更
    1) 原质押将作废，但可将质押 DePC coin 取出
    2) 质押添加锁定期限，锁定期内将无法取出
    3) 质押收益受质押时间影响

**普通用户需要做出变更**

* 更新最新的节点程序

**矿工需要做出变更**

* 更新最新的节点程序

* 使用 Chia 官方的程序初始化硬盘；

* 若有质押的 DePC，先将原质押 DePC 取出；

* 分叉高度达到后，并使用新的挖矿客户端重新进行绑定与质押；

## 准备节点

### 下载或更新你的节点程序

挖矿需要 DePINC 全节点，你需要到官方网站 [https://depinc.org](https://depinc.org)
下载最新版本的节点程序.若你已经是 DePINC 的用户，你也需要到官方网站下载最新版本的
节点程序进行更新。

**下载好了节点程序，双击该安装包将可以进行安装，安装完毕后，建议将 DePINC 的主目
录加到搜索路径以方便执行后续命令.**

**节点程序将会自动从网络上下载所有的区块数据，请务必保证存储节点数据的硬盘剩余空
间至少有 10GB 或更多.**

## 使用 Chia 客户端初始化挖矿硬盘(P盘)

具体请参见 Chia 官网说明：[https://chia.net](https://chia.net)

## 下载挖矿客户端

挖矿的客户端请访问 [depinc-miner](https://github.com/DePINC/miner/releases) 下载
最新的版本，若你使用的系统是 Linux 请下载源代码并按照项目中的指示自行编译。

## 编写挖矿配置文件

下载了新版本的客户端后，请将该压缩包中的所有文件都解开到一个新的文件夹。在其中会
找到一个名为 depinc-miner.exe 的文件，（在 Linux 下该文件存在于 Miner 工程目录下
的 build/depinc-miner ）。该文件为挖矿客户端的主程序.在任何的挖矿工作开始前，都需
要先创建并编写一个 Json 格式的配置文件，一般命名为 `config.json`。

> 这个配置文件可以为任意的名称，但是需要保证其内容是符合需求的。你可以在执行命令
> 时添加参数 `-c othername.json` 来指定该配置文件的名称为 `othername.json`。

执行以下指令将会为你在当前的目录下初始化一个空的配置文件。若不加任何参数，则默认
名称为 `config.json`：

```bash
depinc-miner.exe generate-config
```

或输出指定路径和名称的配置文件：

```bash
depinc-miner.exe -c othername.json generate-config
```

**编辑该文件：**

* reward - 填入一个 DePC 地址，该地址是你的挖矿收益地址；

* seed - 这是你的 Chia 钱包中的助记词，该助记词生成了你的 Farmer public-key，该
  Farmer public-key 与你的 Plots 绑定，挖矿程序需要使用该 Seed 来对新区块进行签名
  以证明挖矿收益属于你。该助记词只用于本地签名，不会被上传或它用。你可以在这里添
  加多组助记词，挖矿程序在扫描硬盘的时候会根据你保存了最好答案的 Plot 文件来使用
  对应的助记词进行签名；

* plotPath - 指向你保存 Plots 的文件夹，该项目是一个数组，可以添加多个文件夹；

* rpc - 挖矿程序与节点程序的连接方式。一般提供三个项目：
  `host`, `username`, `password`。若不提供 `username`， `password`，挖矿程序将会
  去读取 `.cookie` 文件来获取连接认证信息。`.cookie` 文件可以在启动挖矿程序的时候
  通过参数 `--cookie` 去指定，若不指定该文件，则会从默认的目录中寻找；

* timelords - 指定一个或多个 VDF 管理程序，挖矿程序需要通过该程序来获得时间证明，
  请翻阅之后的章节来获得关于 Timelord 的信息。若你不知道如何填写该项目，请使用
  `timelord.bhd.one:29292` 来作为选项，但这并不是最好的选择。

**示例：**

```json
{
    "reward": "38CLnjuj31ifZMXZV8UhbyCo3fNP46Lszy"，
    "seed": "bird convince trend skin lumber escape crater describe ..."，
    "plotPath": [
        "/home/matthew/data/plotfiles1"，
        "/home/matthew/data/plotfiles2"
    ]，
    "rpc": {
        "host": "http://127.0.0.1:18732"
    },
    "timelords": [
        "timelord.bhd.one:29292"
    ]
}
```

## 与Chia的账户公钥绑定

在开始挖矿前，你需要将你的收益地址与你的Chia Farmer公钥绑定到一起并提交该信息到
DePINC 点对点网络。

**以下条件需要被满足：**

* 节点程序同步已经完成；

* 已经完成了 `config.json` 的配置，挖矿程序可以连接到节点程序；

* 虽然绑定操作不需要质押，但是进行转账操作需要主 DePC 账户里有余额；

**执行以下命令，让挖矿程序产生绑定交易并提交到 DePINC 点对点网络：**

```bash
depinc-miner.exe bind
```

正常情况下，该指令执行完成后会显示出转账的交易哈希值。若失败，请检查返回信息并对
配置做出相应的更改。

> 若你的配置文件中包含多个助记词，则可以在绑定的时候通过添加参数 `--index` 来指
> 定要绑定的助记词对应的 Farmer public-key，请将所有的助记词都进行绑定操作。

例如，要绑定第2个助记词，请使用下方的命令：

```bash
depinc-miner.exe bind --index=1
```

> 助记词序号从0开始计算起

## 质押DePC coin以获得挖矿收益最大化

若不进行质押，那么每次出块都会检查收益地址的质押 DePC 的数量，若该数量不达到全网
算力的需求量，则该收益地址无法拿到此次挖矿产出的所有奖励的收益，这些无法被拿到的
收益将会奖励给下一个出块并有质押的矿工。

注：在 Chia 共识协议硬分叉后，质押的 DePC 将会被锁定在网络上一段时间，当质押时间
达到时才可以被取出。

### 如何进行质押？

执行以下命令，让挖矿程序产生质押交易并提交到 DePINC 点对点网络：

```bash
depinc-miner.exe deposit --amount 5 --term term3
```

> 该命令将会质押 5 DePC 到网络上，质押类型为 `term3`。

## 启动挖矿客户端

以上步骤都完成后，则可以使用下方命令启动挖矿客户端：

```bash
depinc-miner.exe mining
```

> 若你的配置文件不在当前目录下，则可以使用 `-c` 来指定配置文件，否则挖矿程序会报
> 错并且退出。

## 运行VDF以获得及时的时间证明数据（只适合Linux）

### 什么是 Timelord 和 VDF？

* VDF 是 Chia 共识协议中的时间证明，一个 VDF proof 可证明为了获取该 proof 一定有
  相应的时间流逝.该求证过程不可以通过并行来进行加速，所以杜绝了 GPU 或专用矿机来
  对 VDF 求证的可能。

* 时间证明将会极大地增加网络的安全性，每个新的区块都标记了一个 VDF 证明，这样让使
  用大算力重新生成全网数据变得非常困难。

* 网络中各 VDF 计算出来的结果都将是一致的，并且不同地方运行的 VDF 不具有竞争性。
  全网中至少要保证有一个 VDF 计算器在运行。

* Timelord 是 VDF 证明程序的管理程序，我们使用它来管理与链上相关的时间证明程序，
  并协调并获取时间证明。

### 在本地运行 VDF 的好处？

* 若在本地不运行 VDF，则需要等待网络中的 VDF proof，可能会因为网络延迟而导致出块
  延迟，进而使得区块无法及时被全网发现；

* 运行一个 VDF 将会把答案提交到全网，会让 DePINC 网络更加稳定和安全；

### 本地运行 VDF 的条件

* 一块速度不差的 CPU，例如：最新款的 i7。

* 使用 Linux 操作系统来运行节点；

> 注1：因为 Chia 官方的 VDF 只针对 Linux 操作系统进行了优化，虽然 Windows 下也可
> 以进行VDF计算，但是速度比起 Linux 下要慢很多，所以我们不建议用户在 Windows 下运
> 行VDF 程序。

> 注2：因为 VDF 的计算无法使用并行进行加速，所以对于 CPU 资源的占用率不会很高，最
> 多单核单线程满载。

### 如何启动VDF计算？

* 访问 Timelord 的代码仓库并获得最新版本的程序。可以访问这个链接获得：
[Timelord](https://github.com/depinc/timelord)

* 访问 DePINC 的 [Wiki](https://github.com/DePINC/depinc/wiki)
  可以获得更多的关于 Timelord 的使用说明。

> 注：同一个challenge的多个VDF proof求解只需要启动一个`vdf_client`，不管收到多少
> 个不同的VDF proof请求，都不会出现计算机资源消耗过大的问题。

### 通过 P2P 传播的 VDF 证明答案

当你的挖矿程序没有连接至任何的 VDF 计算程序，那么它将会把 VDF 运算请求向 P2P 网络
发送出去，若有 Timelord 程序通过节点接收到了该请求，也会在 VDF 答案获取时将相关
的答案发送回 P2P 网络，此时若你能接收到该答案，也可以满足出块的需求。

> 但是通过 P2P 网络来获取 VDF 答案可能会出现不稳定的情况，有时候你也无法判断在网
> 络中是否有 Timelord 机器在运行。
