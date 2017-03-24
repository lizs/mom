# <b>基于libuv的MOM（消息中间件）实现</b>

## <b>特性</b>
* 轻量、高效
* PUSH REQ/REP BROADCAST消息模型实现
* 保活、自动重连
* Signal/Property
* Scheduler（基于uv_timer）
* 内存池

## <b>Getting started</b>
Tcp客户端：
```c++
  auto client = std::make_unique<TcpClient>(ip, port,

                         // session established callback
                         [=](bool success, session_t* session_t) {},

                         // session closed callback
                         [](session_t* session_t) {},

                         // request handler
                         [](session_t* session_t, cbuf_ptr_t pcb, req_cb_t cb) {
                           return false;
                         },

                         // push handler
                         [](session_t* session_t, cbuf_ptr_t pcb) {},

                         // auto reconnect enabled
                         true);

  client->startup();
  RUN_UV_DEFAULT_LOOP();
  client->shutdown();
```
### <b>REQ</b>
```c++
  // 分配请求所需内存
  auto pcb = alloc_request(ops, node_id, buffer_size);
  // 填充请求数据
  pcb->write_binary(reqData, buffer_size);
  // 请求
  client->request(pcb, [](error_no_t error_no, cbuf_ptr_t pcb) {
                      // 请求回调
                      LOG(mom_str_err(error_no));
                  });
```

### <b>PUSH</b>
```c++
  auto pcb = alloc_push(ops, node_id, buffer_size);
  pcb->write_binary(pushData, buffer_size);

  client->push(pcb, [](bool success) {
               });
```
               
### <b>Broadcast</b>
```c++
  auto pcb = alloc_broadcast(buffer_size);
  pcb->write_binary(broadcastData, buffer_size);

  client->push(pcb, [](bool success) {
               });
```

### <b>CircularBuf</b>
循环Buffer，两个作用：<br>
1、为uv_buf提供内存<br>
2、避免在消息的传输链中做无谓的内存拷贝

### <b>Scheduler</b>
定时调度器，基于uv_timer实现。接口如下：
```c++
  timer_id_t invoke(timer_period_t delay, timer_cb_t cb);
  timer_id_t invoke(timer_period_t delay, timer_period_t period, timer_cb_t cb);
  bool cancel(timer_id_t id);
  void cancel_all();
```
示例:
```c++
  Scheduler scheduler;
  // print "hello mom" after 1000 ms
  scheduler.invoke(1000, []() {
    std::cout << "hello mom" << std::endl;
  });
  // print "hello mom" after 1000ms and repeat print every 2000 ms
  auto id = scheduler.invoke(1000, 2000, []() {
    std::cout << "hello mom" << std::endl;
  });
  // stop scheduling
  scheduler.cancel(id);
  // or stop all callbacks binding to scheduler
  scheduler.cancel_all();
```

### <b>Signal</b>
类boost.signals实现<br>
示例：
```c++
  Signal<void()> sig;
  auto cid = sig.conn(print);

  Test test;
  sig.conn(std::bind(&Test::print, test));
  sig.conn([]() {
    std::cout << "Hello, lambda!" << std::endl;
  });
  sig();
  sig.disconn(cid);
  sig();
  sig.disconn_all();
  sig();
```

### <b>Value/Property</b>
Value和Property基于Signal<br>
Value：表达一个可跟踪的值<br>
Property：表达一个可跟踪的值集合<br>
Value示例：<br>
```c++
  Value<int> intger(One, 1);
  intger.conn([](val_id_t vid, const int& val) {
    std::cout << "conn : Value " << vid << " Changed to : " << val << std::endl;
  });
  intger = 2;
  intger = 3;
  intger = 4;
  // no longer callback here
  intger = 4;
```
Property示例：<br>
```c++
  enum Pid{
    One,
    Two,
  };
  
  // 初始一个初值为0的int值包
  Property<int> intgers({Pid::One, Pid::Two}, 0);
  // 跟踪属性One修改
  auto cid2 = intgers.conn(One, [](val_id_t vid, const int& val) {
                             std::cout << "conn : Property " << vid << " Changed to : " << val << std::endl;
                           });
  // 跟踪任何属性修改
  auto gcid = intgers.conn_all([](val_id_t vid, const int& val) {
    std::cout << "conn_all : Property " << vid << " Changed to : " << val << std::endl;
  });

  // 修改One的值为1
  intgers.set(One, 1);
  // 停止对值One的跟踪
  intgers.disconn(One, cid2);
  intgers.set(One, 10);
  intgers.set(Two, 20);
  // 停止对属性包的跟踪
  intgers.disconn_all(gcid);
```
