# 如何在电脑上关闭Hyper-v

(1)在桌面上新建Hyper-V.txt，并粘贴代码

```
pushd "%~dp0"
dir /b %SystemRoot%\servicing\Packages\*Hyper-V*.mum >hyper-v.txt
for /f %%i in ('findstr /i . hyper-v.txt 2^>nul') do dism /online /norestart /add-package:"%SystemRoot%\servicing\Packages\%%i"
del hyper-v.txt
Dism /online /enable-feature /featurename:Microsoft-Hyper-V-All /LimitAccess /ALL
```

然后将文件后缀改成`.cmd`，重启电脑。

(2)搜索电脑上的`启动或关闭Windows功能`，点击Hyper-V关闭。

![image-20250325163613767](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250325163613767.png)

(3)在搜索框搜索`计算机管理`

点击`服务和应用程序->服务`

在服务内找到全部以 Hyper-V 为开头的服务，双击启动类型改为禁用。

![image-20250325164211321](C:\Users\coffe\AppData\Roaming\Typora\typora-user-images\image-20250325164211321.png)

(4)用管理员模式打开命令行并运行关闭代码：

```
以管理员身份打开终端 运行命令
开启：bcdedit /set hypervisorlaunchtype auto 然后重启
关闭：bcdedit /set hypervisorlaunchtype off 然后重启
```

重启电脑