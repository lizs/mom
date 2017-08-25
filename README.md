# <b>基于libuv的MOM（消息中间件）实现</b>

## <b>Feature</b>
* PUSH REQ/REP PUB/SUB pattern
* Keep alive/Auto reconnect
* Signal/Property
* Scheduler（based on uv_timer）
* Memory pool

## <b>Getting started</b>
TcpServer
```c++
// custom handler
class ServerHandler : public VK::Net::IHandler {
  void on_connected(bool success, session_ptr_t session) override { }
  void on_closed(session_ptr_t session) override { }

  void on_req(session_ptr_t session, cbuf_ptr_t pcb, resp_cb_t cb) override {
    if (cb) {
      // echo back
      cb(Success, pcb);
    }
  }

  error_no_t on_push(session_ptr_t session, cbuf_ptr_t pcb) override {
    printf_s(pcb->get_head_ptr());
    return Success;
  }
};

// start server
auto server = std::make_shared<TcpServer>("127.0.0.1", 5002, std::make_shared<ServerHandler>());
server->startup();

// pub
VK::Net::Scheduler scheduler;
scheduler.invoke(1000, 1000, [server](any token) {
  auto pcb = alloc_cbuf(cbuf_len_t(strlen(pubBytes) + 1));
  pcb->write_binary(pubBytes, cbuf_len_t(strlen(pubBytes) + 1));
  server->pub("Channel", pcb);
});

RUN_UV_DEFAULT_LOOP();
server->shutdown();
```
TcpClient
```c++
// custom handler
class ClientHandler : public VK::Net::IHandler {
  void on_connected(bool success, session_ptr_t session) override {
    if (success) {
      // sub
      session->sub("Channel");
      request(session);
    }
  }

  void on_closed(session_ptr_t session) override {	}

  void on_req(session_ptr_t session, cbuf_ptr_t pcb, resp_cb_t cb) override {
    cb(0, nullptr);
  }

  error_no_t on_push(session_ptr_t session, cbuf_ptr_t pcb) override {
    Logger::instance().debug(pcb->get_head_ptr());
    return Success;
  }

  void request(session_ptr_t session) {
    auto pcb = alloc_cbuf(cbuf_len_t(strlen(reqBytes) + 1));
    pcb->write_binary(reqBytes, cbuf_len_t(strlen(reqBytes) + 1));

    session->request(pcb, [this](session_ptr_t session, error_no_t err, cbuf_ptr_t pcb) {
      if (!err)
        request(session);
    });
  }
};

// start client
auto client = std::make_shared<TcpClient>("127.0.0.1", 5002, std::make_shared<ClientHandler>(), true, false);
client->startup();
RUN_UV_DEFAULT_LOOP();
client->shutdown();
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
enum Pid {
	One = 1,
	Two,
	Three,
};

Value intger(One, 1);
Value intger1(Two, 2);

intger.conn([](val_id_t vid, const Value& val) {
  std::cout << "conn : Value " << vid << " Changed to : " << val.to<int>() << std::endl;
});

// will throw exception
// can only be assigned with same type
//intger = intger1;

intger = 2;
intger = 3;
intger = 4;
// no longer callback here
intger = 4;
```
Property示例：<br>
```c++
Property pack;
pack.add<int>(One, 0);
pack.add<float>(Two, 0);
pack.add<std::string>(Three, "Hello world!");

auto cid2 = pack.conn(One, [](val_id_t vid, const Value& val) {
  std::cout << "conn : Property " << vid << " Changed to : ";
  switch (static_cast<Pid>(vid)) {
  case One:
    std::cout << val.to<int>() << std::endl;
    break;
  default:
    break;
  }
});

auto gcid = pack.conn_all([](val_id_t vid, const Value& val) {
  std::cout << "conn_all : Property " << vid << " Changed to : ";
  switch (static_cast<Pid>(vid)) {
  case One:
    std::cout << val.to<int>() << std::endl;
    break;
  case Two:
    std::cout << val.to<float>() << std::endl;
    break;
  case Three:
    std::cout << val.to<std::string>() << std::endl;
    break;
  default:
    break;
  }
});

pack.set(One, 1);
pack.disconn(One, cid2);
pack.set(One, 10);
pack.set(Two, 20.0f);
pack.set<std::string>(Three, "Hello Property!");
pack.disconn_all(gcid);
```
## Question
QQ Group : [点击加入](http://jq.qq.com/?_wv=1027&k=VptNja)<br>
e-mail : lizs4ever@163.com
