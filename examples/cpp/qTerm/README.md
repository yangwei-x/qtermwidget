## 使用方式

### 启动示例（构建目录中）：

```
./qTerm
```

图形界面中：

1. 下拉框选择 Serial 或 SSH。
2. Serial 模式输入设备路径与波特率（如 ttyACM0, 115200），点击 Open。
3. SSH 模式输入 user@host（或直接 host，遵循系统 ssh 配置），点击 Open。

命令行参数（可选）：

```
./qTerm --mode ssh --device user@host
./qTerm --mode serial --device /dev/ttyACM0 --baud 115200
```

当前实现局限

* openPty 简化实现：不会强制终止已有正在运行的进程（假设使用前处于空闲或已退出）。需要更健壮的切换可扩展：
检测并调用 Session 停止 / 重新初始化。
或新建一个独立的 Session 并重新装配 TerminalDisplay。
* 若之后在 serial 会话已打开时切换到 SSH，当前实现不释放 serial 资源（serial 模式设计主要用于单一模式演示）。需要增强时可添加状态跟踪和关闭逻辑。
* 未处理 ssh 不在 PATH 的错误提示（可以通过检查 QStandardPaths::findExecutable("ssh") 增强）。

可选后续改进建议

## SSH 连接问题与排查

1. **无法连接/无输出**：
	- 请确认目标主机可达（可用 `ssh user@host` 在终端测试）。
	- 检查系统是否已安装 `ssh` 客户端（`which ssh`）。
	- 若需指定端口或密钥，可在 Device/Host 输入框中使用 `user@host -p 2222` 或 `user@host -i /path/to/key`（当前仅支持简单参数，复杂参数建议命令行测试）。
	- 若 SSH 需要密码，窗口会弹出密码提示（如未弹出，可能是 ssh 客户端未正确启动）。
	- 若主机密钥未缓存，首次连接可能会有额外提示。
	- 本示例已强制添加 `-tt` 以确保分配伪终端（避免密码提示丢失）。

2. **Unable to load translator "default"**：
	- 此警告不影响 SSH 连接，仅为 Qt 翻译文件未找到。可忽略。

3. **无界面响应/卡死**：
	- 请确认输入的 host 格式正确。
	- 检查本地防火墙或 SSH 配置。

4. **调试建议**：
	- 在终端直接运行 `ssh user@host`，确认能正常连接。
	- 若有特殊参数需求，建议先在终端测试命令。
	- 如果需要完全自定义参数，可修改 `main.cpp` 中构建 `args` 的部分。

## 设计说明（2025-09 更新）

* 示例中创建 `QTermWidget` 时使用 `startnow=0`，不自动启动默认 shell，避免与随后用户点击 Open 时的 SSH/Serial 会话冲突。
* SSH 模式默认传入参数：`ssh -tt <user@host>`。
* 如果未来实现“重复打开”或“关闭旧会话”逻辑，可在点击 Open 前检测 session 运行状态并调用自定义 close API（当前未实现）。

## 依赖说明

- SSH 模式依赖系统 `ssh` 命令（openssh-client）。
- 若未安装请使用 `sudo apt install openssh-client`（Debian/Ubuntu）或对应发行版包管理器。