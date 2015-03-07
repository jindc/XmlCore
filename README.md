# XmlCore
XmlCore是一个用C++编写的Xml解析器，算法参考了《编译原理》（龙书），使用了LL(1)语法制导翻译，可以很方便的扩展为对html的解析器，对于网页抽取和小的html浏览器是不错地选择。
    
XmlCore现在的算法是边解析边创建了dom树。对于大的xml文件场景，可以很方便的修改为基于事件的解析方式。
    
XmlCore是基于acsii编码进行解析的。但是对于GBK,GB2312,GB18030等编码从原理上来说也是支持的，对UTF-8则有问题。

文件介绍：
    XmlCore.h 语法解析
    XmlScanner.h 词法解析器
    XmlUtil.h 一些xml工具函数
    test目录下是测试程序和测试文件，程序经过valgrind 内存测试。
    
词法解析:
dec 
    <?xml dec_props  ?>
dec_props
    null
    dec_prop dec_props
dec_prop
    id="chars"
comnent
    <!-- chars -->|<!-- comment -->
tag_begin
    <id tag_prop>
tag_value
    chars
cdata
    <![CDATA[ chars ]]> 
tag_end
    </id>
id
    chars
tag_prop
    chars
    
语法解析：    
 doc
    null
    root
    dec root
root
    null
    tag

tag  {tag_begin.id = tag_end.id}
    tag_begin null tagend
    tag_begin tag tagend
    tag_begin cdata tag_end  
    tag_begin val tagend
tag_begin
tag_end

编译及运行：
  ./build.sh
  cd test
  ./test example.xml
  
作者：德才
email:jindc@163.com
