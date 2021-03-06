# 实验3：自制文件系统 sfs (simple file system)

## 正确打开方式

```bash
make
./sfs mountpoint
```

## 实现思路：自上而下

### 内存分配

用 4G/8K 个block存储信息，每个BLOCK存储8k的空间。使用mmap分配，程序运行中用munmap释放\
在block[0]中存储文件系统的元信息：文件数，块使用状况。\
block[1]中存储文件元信息：文件名，使用了哪些块。\
其余的block用来存储文件内容。

### 文件系统管理：空间

sfs使用了一个位图记录块是否被使用，其中0号块和1号块是默认使用的，用来存储元信息。而其他的块如果使用了，则将相应的位设置为0，未使用则设置为1。

### 文件系统管理：文件

在block[1]中存储文件元信息，包括最多31个字符的文件名，文件状态和文件使用了哪些块。

### 文件系统管理：存储结构

文件系统中采用了类似inode的3层索引表，索引表中存的是块序号。\
其中0级表存31个块地址加一个1级表地址，1级表中存了2047个块地址加一个2级表地址，2级表中存了2048个1级表地址，到2级表中已经可以存储32G的信息，已无再增加的必要。

### 文件管理：新建

调用mknod和open方法，因为并没有权限管理，所以open函数是默认返回0(success)的，mknod函数会在block[1]末尾添加新的文件信息节点，并将size设置为0，更新block[0]中的文件系统元信息。

### 文件管理：写入

写入的时候先要得到offset在文件中对应哪一块，可通过一个函数计算得到，如果函数返回0，则表示已经达到文件末尾，需要分配新的块，分配新的块之后在其中写入不超过8k的数据，重复执行直到全部写入。

### 文件管理：读取

与写入过程类似，先要得到offset在文件中具体的块，从块中读出信息，如果达到文件末尾（返回0）则停止读取并返回读的字节数。

### 文件管理：截断

截断有2种情况：变短和变长，如果变短，需要得到截断点对应的块地址，字节偏移，然后释放文件后面的存储块和索引块并更新文件元信息。\
如果是变长，则需要分配新的块并更新文件元信息。

### 文件管理：删除

截断为0后再将元信息也删除，并更新block[0]中的文件系统元信息。

### 块管理：分配

分配块需要3步操作：

1. 得到新的未使用的块。
2. 将块地址写入文件元信息中。
3. 将对应位图中的位改为0。 //tudo:错误处理。

### 块管理：释放

1. 将块的内容清空（其实不是必要的。）
2. 将文件元信息中对该块的引用删除。
3. 在位图中将对应的位写为1。
4. 将块释放。(munmap)

## 一些缺陷

1. 一些函数的重复调用，应该可以优化。//不想优化了。
2. ls -l 的输出文件大小有问题。应该是getattr目录时得到的信息不全。
3. 还不能rename，就不能实现mv操作。
4. 目前一个目录里最多只能有32个link。

## 实现目录操作改变的东西

### 文件系统层面

1. 目录作为文件保存，增加目录和一般文件标志。
2. 目录中的l0_block存储的是其子文件（目录或普通文件）的文件节点序号。
3. 搜索文件变成由根结点向下逐层搜索//done
4. 新建文件时，要分配新的位置，并更新父节点元信息。（link数目，下属索引）
5. 删除文件目录，要检测目录是否为空，空目录才能删除。
6. 删除文件节点时并不移动其他节点。而采用遍历手段。