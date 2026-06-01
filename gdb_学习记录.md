# GDB 学习记录

这份文档基于当前项目里一次真实的教学型崩溃，记录从 `debug` 编译、进入 `gdb`、打断点、查看变量、定位崩溃到阅读调用栈的完整过程，方便后续重复参考。

## 1. 本次练习的目标

本次练习不是随便看几个命令，而是围绕一次真实崩溃，学会回答下面几个问题：

- 程序现在停在哪一行
- 当前局部变量是什么值
- 程序为什么会进入某个分支
- 程序真正崩溃在哪一层
- Qt 库里的崩溃，怎么回到自己的源码层定位

## 2. 本次练习的 Bug 放在哪里

为了便于学习，故意在 `功放设置` 页的 `查询` 按钮点击路径里放了一个教学型崩溃。

相关代码位置：

- 查询按钮连接到槽函数：
  - `src/views/settings/power_amplifier/power_amplifier_page.cpp`
- 教学函数：
  - `PowerAmplifierPage::triggerTrainingBug()`

核心逻辑如下：

```cpp
connect(queryButton, &QPushButton::clicked, this, &PowerAmplifierPage::triggerTrainingBug);

void PowerAmplifierPage::triggerTrainingBug()
{
    const QStringList paNames = {"PA1", "PA2", "PA3", "PA4", "PA5", "PA6"};
    const int requestedChannel = 6;
    const bool indexValid = requestedChannel >= 0 && requestedChannel < paNames.size();

    QLabel *resultLabel = nullptr;

    if (!indexValid)
    {
        resultLabel->setText(QStringLiteral("无效通道: %1").arg(requestedChannel));
        return;
    }

    resultLabel->setText(paNames.at(requestedChannel));
}
```

这个崩溃的目的很明确：

- `requestedChannel = 6`
- `paNames.size() = 6`
- 合法下标范围是 `0..5`
- 所以 `indexValid = false`
- 程序进入 `if (!indexValid)`
- `resultLabel` 又是 `nullptr`
- 调用 `resultLabel->setText(...)` 时触发 `SIGSEGV`

## 3. 为什么先要编译 Debug 版本

如果直接 `gdb ./qt_test`，但程序没有带调试符号，通常会看到：

```text
Reading symbols from ./qt_test...(no debugging symbols found)...done.
```

这种情况下也能调，但体验很差：

- 看不到清晰的源码行号
- 很多变量名和函数信息不完整
- 定位效率会明显下降

所以先用 Debug 方式重新编译。

项目里已经准备了脚本：

```bash
./run_debug.sh
```

这个脚本会自动做三件事：

- `make clean`
- `qmake "CONFIG+=debug" qt_test.pro`
- `make -j4`

编完后再进 `gdb`，正常会看到：

```text
Reading symbols from ./qt_test...done.
```

这说明调试符号已经准备好。

## 4. 本次实际使用的基本流程

### 4.1 进入 gdb

```bash
gdb ./qt_test
```

### 4.2 设置显示环境

这个项目是 Qt 图形程序，通常需要设置显示环境：

```gdb
set env DISPLAY :0
```

作用：

- 告诉被调试程序图形输出到哪里
- 不设置时，有可能界面起不来或者看不到

### 4.3 在目标函数打断点

```gdb
break PowerAmplifierPage::triggerTrainingBug
```

这条命令的意思是：

- 在 `PowerAmplifierPage::triggerTrainingBug()` 这个函数入口处停住

为什么能这样写：

- 因为 Debug 编译后，`gdb` 能从调试符号里找到：
  - 类名
  - 函数名
  - 文件名
  - 行号
  - 机器码地址

`gdb` 通常会返回类似：

```text
Breakpoint 1 at 0x67f40: file src/views/settings/power_amplifier/power_amplifier_page.cpp, line 92
```

这说明：

- 断点编号是 `1`
- 函数入口地址找到了
- 对应源码文件和源码行也找到了

### 4.4 运行程序

```gdb
run
```

程序启动后，在界面里执行：

- 登录 `root`
- 打开 `功放设置`
- 点击 `查询`

因为按钮点击已经连接到 `triggerTrainingBug()`，所以程序会在进入该函数时被断住。

## 5. 这次重点学习的命令和解释

### 5.1 `list`

用法：

```gdb
list
list 91,106
```

作用：

- 显示当前停住位置附近的源码

为什么要用：

- 调试前先确认当前到底停在哪
- 看到当前行上下文，才知道下一步该 `next` 还是 `print`

这次实际查看的源码范围就是：

- `PowerAmplifierPage::triggerTrainingBug()` 附近几行

注意：

- `list` 只是显示源码
- 它不会推动程序执行
- 多次执行 `list`，通常会继续往后翻源码

### 5.2 `next`

用法：

```gdb
next
```

作用：

- 执行当前这一行，然后停到下一行

为什么要用：

- 这是最适合入门的“按行单步”
- 它不钻进函数内部，适合先掌握当前逻辑流向

本次练习里，通过多次 `next`，确认了变量逐步完成初始化：

- `paNames`
- `requestedChannel`
- `indexValid`
- `resultLabel`

### 5.3 `info locals`

用法：

```gdb
info locals
```

作用：

- 一次性显示当前函数里的局部变量

为什么要用：

- 刚进函数时，不一定清楚有哪些变量值得观察
- 先扫一眼局部变量列表，能快速建立当前现场概念

本次实际看到的关键信息包括：

- `requestedChannel = 6`
- `indexValid = false`
- `resultLabel` 在某一时刻还是旧值

注意：

- 如果当前停在某一行，而这一行还没执行，局部变量可能还是旧值或未初始化值
- 所以 `info locals` 要结合当前停住的位置去解读

### 5.4 `print`

用法：

```gdb
print requestedChannel
print paNames.size()
print indexValid
print resultLabel
```

作用：

- 打印某个变量或表达式的当前值

为什么要用：

- 调试不是猜，而是验证
- 这几条正好在验证四个关键问题：

1. `requestedChannel`
   - 当前要访问哪个通道

2. `paNames.size()`
   - 容器长度是多少

3. `indexValid`
   - 条件判断结果是什么

4. `resultLabel`
   - 指针是不是空

本次练习里，最终确认：

- `requestedChannel = 6`
- `paNames.size() = 6`
- `indexValid = false`
- `resultLabel = 0x0`

### 5.5 `bt`

用法：

```gdb
bt
```

全称：

- `backtrace`

作用：

- 打印当前调用栈

为什么要用：

- 崩溃时不能只看“当前死在哪”
- 更重要的是看“它是怎么一路调用到这里的”

本次崩溃里，调用栈中最关键的层有：

- `#0 QLabel::setText(...)`
- `#1 PowerAmplifierPage::triggerTrainingBug(...)`
- 再往上是 Qt 的信号槽分发链路

从这里能看出：

- 表面崩在 Qt
- 根因在自己的 `triggerTrainingBug()` 里

### 5.6 `frame 0`

用法：

```gdb
frame 0
```

作用：

- 切到调用栈的第 0 层，也就是当前崩溃层

为什么要用：

- `bt` 显示的是整条调用栈
- `frame` 让你能聚焦某一层

本次 `frame 0` 后，对应的是：

- `QLabel::setText(QString const&)`

也就是程序真正炸掉的那一层。

### 5.7 `up`

用法：

```gdb
up
```

作用：

- 在调用栈里往调用者方向移动一层

为什么要用：

- 崩溃经常发生在 Qt 或系统库里
- 但真正需要修改的是你自己的上一层代码

本次 `up` 后，就从：

- `QLabel::setText(...)`

回到了：

- `PowerAmplifierPage::triggerTrainingBug(...)`

这一步非常关键，因为它把你从“库函数层”带回“业务代码层”。

### 5.8 `down`

用法：

```gdb
down
```

作用：

- 在调用栈里往更深一层被调用者方向移动

为什么要知道：

- 如果你 `up` 回到了自己的代码层
- 想再回到崩溃层看细节，就可以 `down`

这次练习里没有重点使用它，但它和 `up` 是一对常用命令。

## 6. 这次练习最重要的调试原则

### 6.1 当前停在某一行时，这一行通常还没执行

这是本次最关键的认知之一。

例如当程序停在：

```cpp
QLabel *resultLabel = nullptr;
```

这时：

- `requestedChannel` 已经执行完了
- `indexValid` 已经执行完了
- 但 `resultLabel = nullptr` 还没执行

所以这时去看 `resultLabel`，可能看到的是：

- 栈上旧值
- 未初始化残留值

而不是 `0x0`

只有再执行一次：

```gdb
next
```

这句真正跑完后，再 `print resultLabel`，你才看到：

```gdb
(QLabel *) 0x0
```

### 6.2 不要只看崩溃点，要看因果链

这次不是简单一句“空指针崩了”就结束。

完整因果链是：

1. `requestedChannel = 6`
2. `paNames.size() = 6`
3. 合法下标范围只有 `0..5`
4. 所以 `indexValid = false`
5. 程序进入 `if (!indexValid)`
6. 此时 `resultLabel = nullptr`
7. 调用 `resultLabel->setText(...)`
8. 触发 `SIGSEGV`

真正的调试目标，是把这条链完整串起来。

### 6.3 崩在 Qt 不等于问题在 Qt

本次 `bt` 最顶层是：

- `QLabel::setText(...)`

但问题不是 Qt 自己坏了，而是：

- 我们把一个空指针当成 `QLabel` 去调用成员函数

所以：

- `frame 0` 看“表面炸点”
- `up` 回“自己代码层”
- 才能找到真正根因

## 7. 本次完整操作记录模板

以后如果再次练类似崩溃，可以直接照着这个顺序走。

### 7.1 Debug 编译

```bash
./run_debug.sh
```

### 7.2 进入 gdb

```bash
gdb ./qt_test
```

### 7.3 设置环境并打断点

```gdb
set env DISPLAY :0
break PowerAmplifierPage::triggerTrainingBug
run
```

### 7.4 在界面中触发

- 登录 `root`
- 打开 `功放设置`
- 点击 `查询`

### 7.5 断住后先看源码

```gdb
list 91,106
```

### 7.6 单步执行到变量初始化之后

```gdb
next
next
next
next
```

### 7.7 打印关键变量

```gdb
info locals
print requestedChannel
print paNames.size()
print indexValid
print resultLabel
```

### 7.8 再走一步，让 `resultLabel = nullptr` 生效

```gdb
next
print resultLabel
print indexValid
```

### 7.9 继续执行到崩溃

```gdb
next
```

### 7.10 崩溃后看调用栈

```gdb
bt
frame 0
up
list
```

## 8. 本次最后确认的结论

本次崩溃已经确认：

- 断点命中了 `PowerAmplifierPage::triggerTrainingBug()`
- `requestedChannel = 6`
- `paNames.size() = 6`
- `indexValid = false`
- `resultLabel = 0x0`
- 程序在 `resultLabel->setText(...)` 处崩溃
- `bt` 显示表面崩在 `QLabel::setText(...)`
- `up` 后确认根因在自己的业务函数里

最终结论：

- 这是一次典型的空指针解引用引起的 `SIGSEGV`
- 并且通过这次练习，完整走通了：
  - `break`
  - `run`
  - `list`
  - `next`
  - `info locals`
  - `print`
  - `bt`
  - `frame`
  - `up`

## 9. 后续建议

建议下一步继续练以下内容：

- `step` 和 `next` 的区别
- `continue`
- 条件断点
- `watch` 观察点
- 修掉这个教学 bug 后，再用 `gdb` 验证“不再崩溃”

如果后续再新增调试经验，也可以继续补在这份文档里，逐渐形成自己的项目调试手册。
