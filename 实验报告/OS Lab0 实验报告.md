# OS Lab0 实验报告 

## 思考题

### Thinking 0.1 思考下列有关 Git 的问题： 

- 在前述已初始化的 ~/learnGit 目录下，创建一个名为 README.txt 的文件。执行命令 git status > Untracked.txt（其中的 > 为输出重定向，我们将在 0.6.3 中 详细介绍）

  ```shell
  git@21231025:~/test/learnGit $ git status > Untracked.txt
  git@21231025:~/test/learnGit $ cat Untracked.txt 
  位于分支 master
  
  尚无提交
  
  未跟踪的文件:
    （使用 "git add <文件>..." 以包含要提交的内容）
          README.md
          Untracked.txt
  
  提交为空，但是存在尚未跟踪的文件（使用 "git add" 建立跟踪）
  ```

- 在 README.txt 文件中添加任意文件内容，然后使用 add 命令，再执行命令 git status > Stage.txt。

  ```shell
  git@21231025:~/test/learnGit $ git add README.md
  git@21231025:~/test/learnGit $ git status > Stage.txt
  git@21231025:~/test/learnGit $ cat Stage.txt
  位于分支 master
  
  尚无提交
  
  要提交的变更：
    （使用 "git rm --cached <文件>..." 以取消暂存）
          新文件：   README.md
  
  未跟踪的文件:
    （使用 "git add <文件>..." 以包含要提交的内容）
          Stage.txt
          Untracked.txt
  
  ```

- 提交 README.txt，并在提交说明里写入自己的学号。

  ```shell
  git@21231025:~/test/learnGit $ git commit -m "21231025"
  [master （根提交） 5956b06] 21231025
   1 file changed, 1 insertion(+)
   create mode 100644 README.md
  ```

- 执行命令 cat Untracked.txt 和 cat Stage.txt，对比两次运行的结果，体会 README.txt 两次所处位置的不同。

  ```shell
  git@21231025:~/test/learnGit (master)$ cat Untracked.txt 
  位于分支 master
  
  尚无提交
  
  未跟踪的文件:
    （使用 "git add <文件>..." 以包含要提交的内容）
          README.md
          Untracked.txt
  
  提交为空，但是存在尚未跟踪的文件（使用 "git add" 建立跟踪）
  git@21231025:~/test/learnGit (master)$ cat Stage.txt 
  位于分支 master
  
  尚无提交
  
  要提交的变更：
    （使用 "git rm --cached <文件>..." 以取消暂存）
          新文件：   README.md
  
  未跟踪的文件:
    （使用 "git add <文件>..." 以包含要提交的内容）
          Stage.txt
          Untracked.txt
  ```

  在使用add之前，README.md在工作区，使用之后放到了暂存区

- 修改 README.txt 文件，再执行命令 git status > Modified.txt。

  ```shell
  git@21231025:~/test/learnGit (master)$ git status > Modified.txt
  ```

- 执行命令 cat Modified.txt，观察其结果和第一次执行 add 命令之前的 status 是 否一样，并思考原因。

  ```shell
  git@21231025:~/test/learnGit (master)$ cat Modified.txt 
  位于分支 master
  尚未暂存以备提交的变更：
    （使用 "git add <文件>..." 更新要提交的内容）
    （使用 "git restore <文件>..." 丢弃工作区的改动）
          修改：     README.md
  
  未跟踪的文件:
    （使用 "git add <文件>..." 以包含要提交的内容）
          Modified.txt
          Stage.txt
          Untracked.txt
  
  修改尚未加入提交（使用 "git add" 和/或 "git commit -a"）
  ```

  README.md状态时已修改，并不在未追踪区，因为使用了add命令将其追踪。

### Thinking 0.2 仔细看看0.10，思考一下箭头中的 add the file 、stage the file 和 commit 分别对应的是 Git 里的哪些命令呢？ 

- add the file

  `git init`

- stage the file

  `git add`

- commit

  `git commit`

### Thinking 0.3 思考下列问题： 

1. 代码文件 print.c 被错误删除时，应当使用什么命令将其恢复？ 

   `git checkout -- print.c`

2. 代码文件 print.c 被错误删除后，执行了 git rm print.c 命令，此时应当 使用什么命令将其恢复？ 

   ```shell
   git reset HEAD --print.c
   git checkout --print.c
   ```

3. 无关文件 hello.txt 已经被添加到暂存区时，如何在不删除此文件的前提下将其移出暂存区？

   `git rm hello.txt`

### Thinking 0.4 思考下列有关 Git 的问题：

- 找到在 /home/22xxxxxx/learnGit 下刚刚创建的 README.txt 文件，若不存 在则新建该文件。

- 在文件里加入 Testing 1，git add，git commit，提交说明记为 1。

- 模仿上述做法，把 1 分别改为 2 和 3，再提交两次。

- 使用 git log 命令查看提交日志，看是否已经有三次提交，记下提交说明为 3 的哈希值a。

  ```shell
  git@21231025:~/test/learnGit (master)$ git log
  commit 7032bac25ea82fcfaa44f8ff07c5ca40804ffa5f (HEAD -> master)
  Author: 钟昊翔 <21231025@buaa.edu.cn>
  Date:   Thu Mar 14 10:26:41 2024 +0800
  
      3
  
  commit 2f1bd875d927621c220dc101ebc1aa35fd05bfc9
  Author: 钟昊翔 <21231025@buaa.edu.cn>
  Date:   Thu Mar 14 10:26:04 2024 +0800
  
      2
  
  commit 734f99675fbf8bdd908d59dadeed2ed8be06783a
  Author: 钟昊翔 <21231025@buaa.edu.cn>
  Date:   Thu Mar 14 10:25:49 2024 +0800
  
      1
  
  commit 5956b06ac95ae631e17706438832ec1ee4fe7c93
  Author: 钟昊翔 <21231025@buaa.edu.cn>
  Date:   Thu Mar 14 10:13:20 2024 +0800
  
      21231025
  ```

- 进行版本回退。执行命令 git reset --hard HEAD^ 后，再执行 git log，观 察其变化。

  ```shell
  commit 2f1bd875d927621c220dc101ebc1aa35fd05bfc9 (HEAD -> master)
  Author: 钟昊翔 <21231025@buaa.edu.cn>
  Date:   Thu Mar 14 10:26:04 2024 +0800
  
      2
  
  commit 734f99675fbf8bdd908d59dadeed2ed8be06783a
  Author: 钟昊翔 <21231025@buaa.edu.cn>
  Date:   Thu Mar 14 10:25:49 2024 +0800
  
      1
  
  commit 5956b06ac95ae631e17706438832ec1ee4fe7c93
  Author: 钟昊翔 <21231025@buaa.edu.cn>
  Date:   Thu Mar 14 10:13:20 2024 +0800
  
      21231025
  ```

  版本进行了回退，到了第三次提交之前

- 找到提交说明为 1 的哈希值，执行命令 git reset --hard  后，再执 行 git log，观察其变化。

  ```shell
  git@21231025:~/test/learnGit (master)$ git log
  commit 734f99675fbf8bdd908d59dadeed2ed8be06783a (HEAD -> master)
  Author: 钟昊翔 <21231025@buaa.edu.cn>
  Date:   Thu Mar 14 10:25:49 2024 +0800
  
      1
  
  commit 5956b06ac95ae631e17706438832ec1ee4fe7c93
  Author: 钟昊翔 <21231025@buaa.edu.cn>
  Date:   Thu Mar 14 10:13:20 2024 +0800
  
      21231025
  ```

  现在回到了第一次提交

- 现在已经回到了旧版本，为了再次回到新版本，执行 git reset --hard  ，再执行 git log，观察其变化。

  现在回到了第三次提交之后，和没有执行`git reset`之前一样

### Thinking 0.5 执行如下命令

```shell
git@21231025:~/test $ echo first
first
git@21231025:~/test $ echo second > output.txt
git@21231025:~/test $ cat output.txt
second
git@21231025:~/test $ echo third > output.txt
git@21231025:~/test $ cat output.txt 
third
git@21231025:~/test $ echo forth >> output.txt
git@21231025:~/test $ cat output.txt 
third
forth
//
>是重定向输出，会覆盖源文件
>>则是追加，不会覆盖，会写在文件末尾
```

### Thinking 0.6 

```shell
git@21231025:~/test $ echo echo shell start
echo shell start
git@21231025:~/test $ echo `echo shell start`
shell start
//
如果用`...`括起来，就会将里面命令的输出当做外面命令的输入，如果不括起来，则是当成普通字符串
echo echo $c>file1
屏幕上不会有显示，file1中则会有 echo 3
echo `echo $c>file1`
屏幕上会有空行一行，file1中会有 3
```

## 难点分析

本次实验最难的地方在extra的最后一问，在实验的过程中由于我不知道使用反引号括起来命令可以将指令的输出赋值给变量，因为花费了大量时间尝试在`awk`命令中编写代码。但是在使用`awk`的过程中对于循环的理解还不够深刻，在使用`man`查看说明的时候也没太看懂。

最终在课下实现了最后一问的效果：

```bash
while [ $PID -ne 0 ]
do
    PID=$[`awk -v input=$PID '$2==input {print $3}' $FILE`]
    echo $PID
done
```

## 实验体会

在这次实验限时上机之前，我只完成了Exercise的内容和课下内容，但是对于指导书中的介绍和Thinking内容没有很好的阅读、理解，导致在课上的时候花费了大量时间阅读指导书和查看指令文档，并且调试指令，尝试运行，浪费了很多时间。几乎实验最后的一个半小时都是在尝试最后一问。我对编写shell脚本和awk指令还不是很熟悉，在课下多次练习后应该已经有所长进。在之后的实验中，我会在课下认真阅读指导书，并且完成书中的所有练习。

