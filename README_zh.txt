使用C99实现的基于POSIX和WIN32的轻量级跨平台C扩展库

基础头文件：
    x/macros.h       宏函数
    x/types.h        类型定义
    x/errno.h        错误处理
    x/assert.h       静态断言和动态断言
    x/detect.h       编译环境探测宏
    x/dump.h         数据结构可视化转储
    x/flowctl.h      流程控制语句
    x/memory.h       管理内存分配
    x/narg.h         参数数量计算
    x/trick.h        宏模板函数
    x/time.h         高精度时间计算
    x/test.h         单元测试
    x/twister.h      MT19937随机数发生器

文件操作：
    x/path.h         文件路径操作
    x/file.h         文件读写
    x/log.h          日志
    x/sys.h          文件系统
    x/stat.h         获取文件元数据
    x/dir.h          读取文件夹

字符串操作：
    x/unicode.h      UNICODE操作
    x/uchar.h        跨平台字符串
    x/printf.h       字符串格式化打印
    x/string.h       串操作函数
    x/cliarg.h       CLI参数解析
	x/regex.h        正则表达式

数据表示：
    x/json.h         Json字符串解析
    x/jpath.h        JsonPath路径检索
    x/base64.h       Base64编解码
    x/ini.h          INI配置文件

数据结构：
    x/btnode.h       二叉树
    x/bitmap.h       位图
    x/heap.h         堆（优先队列）
    x/hmap.h         散列表
    x/list.h         链表
    x/splay.h        伸展树
    x/index.h        索引
    x/pipe.h         FIFO队列
    x/rope.h         绳索
    x/strbuf.h       动态连续字符串

线程和进程：
    x/cond.h         条件变量
    x/mutex.h        互斥量
    x/once.h         原子初始化
    x/thread.h       线程操作
    x/tss.h          线程本地存储
    x/tpool.h        线程池
    x/future.h       Future/Promise模型
    x/proc.h         进程执行
    x/lib.h          加载动态库

终端操作：
    x/tsignal.h      进程/终端信号
    x/tcolor.h       终端颜色支持
    x/edit.h         终端行编辑器

网络操作：
    x/event.h        事件操作
    x/reactor.h      响应器模式
    x/socket.h       套接字操作
    x/sockmux.h      套接字多路复用

密码学：
    x/aes.h         AES对称加密算法
    x/sha256.h      SHA256摘要算法
    x/random.h      生成安全的随机数
