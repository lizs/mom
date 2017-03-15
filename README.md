# <b>MOM based on libuv</b>

## <b>Features</b>
* lightweight
* efficiant
* PUSH REQ/REP pattern
* keep alive
* auto reconnect
* simple interfaces easy to use
* signal/timer

## <b>Getting started</b>
```c++
  client = new TcpClient(ip, port,
	                       // session_t established callback
	                       [=](bool success, session_t* session_t) {
	                       },

	                       // session_t closed callback
	                       [](session_t* session_t) {},

	                       // request handler
	                       [](session_t* session_t, cbuf_ptr_t pcb, req_cb_t cb) {
		                       return false;
	                       },
	                       // push handler
	                       [](session_t* session_t, cbuf_ptr_t pcb) { },
	                       true);

	client->startup();
	RUN_UV_DEFAULT_LOOP();
	client->shutdown();

	delete client;
```
### <b>REQ</b>
```c++
  auto pcb = alloc_request(ops, node_id, buffer_size);
	pcb->write_binary(reqData, buffer_size);

	client->request(pcb, [](error_no_t error_no, cbuf_ptr_t pcb) {
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
