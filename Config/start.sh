#! /usr/bin/bash

CURRENT_PATH=`pwd -P`
EXE_NAME=EasyRouter
CURR_EXE=$CURRENT_PATH/$EXE_NAME

#PYTHONPATH 环境变量
export PYTHONPATH=$CURRENT_PATH

ps_out=(`ps -ef | grep $EXE_NAME | grep -v 'grep' | grep -v $0 | awk '{print $2}'`)

for procid in ${ps_out[@]}
do
     exeinfo=(`ls /proc/$procid/exe -l`)
     if [ "${exeinfo[10]}" == "$CURR_EXE" ]; then
         echo 发现重复进程 $procid，自动杀死进程
         kill -9 $procid
     fi    
done

$CURR_EXE -v
$CURR_EXE >> $CURRENT_PATH/log/cmd_log.txt 2>&1 &