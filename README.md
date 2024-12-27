### 任务要求

实现一个原型图数据库系统，以支撑 LDBC SNB 查询 IC1、IC2、IS1 的存储过程。

LDBC SNB 是一套面向社交网络场景的图查询基准测试（benchmark）。
官网：https://ldbcouncil.org/benchmarks/snb-interactive/
官方 GitHub 文档仓库：https://github.com/ldbc/ldbc_snb_docs
完成本作业不必对 LDBC SNB 有完整的了解，只需参考课程给出的材料即可。

### 作答方式

- driver.cpp ：补全加载数据集的部分代码（注释 [FILL HERE] Load the dataset according to the scale factor 处）；本文件中其他代码不允许修改
- PProcedure.cpp ：参考 ic1、is1 函数，补全 ic2 的实现；ic1、is1 函数实现仅供参考，可以随意修改
- Node.[h|cpp] ：补全成员函数的实现，也可随意修改接口
- Value.[h|cpp] ：用于兼容多种数据类型的类，不允许修改

如果发现代码有任何问题，请发邮件到 michelle.py@pku.edu.cn ，24 小时内应答
面测后需要提交代码包，可能会对代码修改情况、代码与文件之间的契合程度进行抽查和后续的线上沟通。

### 作答进度

1. `2024/12/27`: 由于 Github 对于大文件的限制，不上传数据集至 Github 仓库。开发者请自行在本地准备好数据集。



### 指导

编译方式（CMakeLists.txt 可随意修改）：

```bash
$ cmake .
$ make -j
```

driver 执行方式：

```bash
$ ./driver <sf in {0.1, 3}>
```

例如 `./driver 0.1` 就应该加载 scale factor = 0.1 的数据并执行相应查询。

执行上述命令之后，在控制台交互式键入以执行测试。

测试分两种：

- `builtin_test` ：执行预存了正确结果（ground_truth/ 文件夹；下面将介绍其中的文件格式）的三类查询，并比对代码执行的结果与正确结果。
- interactive ：通过在一行内输入查询类型和参数来执行相应查询并输出结果，用于面测时随机给定新参数测试。输入格式如：`ic1 10995116290806 Joseph`

ground_truth/ 下文件格式：

文件名：[查询类型]-sf[0.1|3].txt

文件内容：分为多段，每一段表示该类查询一组查询参数及其正确结果。每一段的格式如下：

- 查询参数（以空格分隔）
- 正确结果行数 k
- k 行正确结果，每行的各列以 | 分隔

例如 ic1-sf3.txt 中的第一段：

```
10995116290806 Joseph
20
26388279078510|Abdullah|2|625881600000|1330348486449|male|Chrome|188.95.65.197|[Joseph26388279078510@gmail.com, Joseph26388279078510@gmx.com, Joseph26388279078510@hotmail.com]|[ar, en]|Amman|[[World_Islamic_Sciences_and_Education_University, 2011, Amman]]|[[Teebah_Airlines, 2013, Jordan], [Royal_Falcon, 2013, Jordan], [Meelad_Air, 2012, Jordan]]
(...此后还有 19 行)
```

