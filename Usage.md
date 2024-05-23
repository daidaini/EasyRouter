# HDGwRouter使用说明

## 服务配置项说明

配置文件为 _config.json_，跟可执行文件目录保持一致。
相关说明在下面进行基本描述:

- "_server_port_" 是路由服务的监听端口，如果不配置，可以在启动选项中'-p'指定
- "_server_thread_cnt_" 配置服务端处理请求的工作线程数量，如果不配置，可以在启动选项中'-t'指定
- "_dst_addrs_" 配置需要路由转发到的交易网关的地址信息
- "_dst_addrs.group_" 是配置具体对应的我们哪个模块的网关，(选项可为HST2、DDA5等网关模块标识)
- "_dst_addrs.biz_" 用以配置是什么业务，期权(option)还是股票(stock),或是两融(margin)
- "_router_auth_" 是用于配置如何获取路由规则的信息
- "_router_auth.type_" 是配置当前使用哪种路由规则，类型1标识跟恒生系统确认路由目标，类型2是通过本地脚本"**check_local_rule.py**"的"**check_account**"方法确定路由目标
- "_router_auth.third_system_items_" 配置三方模块认证需要的额外的字段名及字段值， 当前用于恒生认证配置需要的固定字段数据信息。
- "_router_auth.thread_cnt_" 配置用于"_router.auth_"的线程池大小
- "_logger_lvl_" 配置日志级别，从高到低有以下几种:
  - "error"
  - "warn"
  - "info"
  - "debug"

### dst_addrs.group 定义

- HST2 (恒生T2网关)
- DDA5 (顶点A5网关)
- DD20 (顶点A2网关)
- HST3 (恒生T3网关)
- HTS_Option (HTS期权网关)
- HTS_Stock (HTS股票网关)
- JSD (金仕达期权网关)
- JSD_Stock (金仕达股票网关)
- ......

### dst_addrs.biz 定义

- option (期权)
- stock (股票)
- margin (两融)

### router_auth.type 定义

- 1 - (恒生T2认证)
- 2 - (本地脚本按账号进行路由规则确认)

> 配置2 的时候，确保部署有"check_local_rule.py"文件，文件名不可更改，  
> 且确保脚本中有"check_account"方法的正确实现。

## 启动

提供了启动脚本start.sh，  
主要是程序的启动命令：  

```shell
#启动
CURRENT_PATH=`pwd -P`
EXE_NAME=HDGwRouter
CURR_EXE=$CURRENT_PATH/$EXE_NAME

$CURR_EXE >> $CURRENT_PATH/log/cmd_log.txt 2>&1 &
#或者
$CURR_EXE --port=30002 --th_size=1 >> $CURRENT_PATH/log/cmd_log.txt 2>&1 &
```

如果 _router_auth.type_ 配置了2，使用本地的python3脚本，则需要额外设置下 **_PYTHONPATH_** 环境变量，在脚本的启动命令前，加一行命令如下：

```shell
#PYTHONPATH 设置环境变量
export PYTHONPATH=$CURRENT_PATH
```

这样，只要保证check_local_rule.py文件在程序目录下即可找到

## 中信证券的全局回切

通过 **backcut** 文件来确认当前系统是否需要回切 ：

- 正常运行情况下，没有该文件，路由服务根据恒生返回的迁移标识进行网关目标的路由
- 当确定需要进行全局回切，则在程序目录下增加 **backcut** 内容，并确保文件内容是1
- 在已经全局回切，当前目录下已经包含内容1的 **backcut** 回切文件时，需要还原到正常路由，则可以有以下两种操作方式：  
    1. 删除 **backcut** 文件
    2. 将 **backcut** 文件内容修改，比如改为0
