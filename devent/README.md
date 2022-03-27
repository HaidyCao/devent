# Devent

## Socket Event

    static void on_read(DocketEvent *event, void *arg) {}

    static void on_write(DocketEvent *event, void *arg) {}

    static void on_event(DocketEvent *event, int what, void *arg) {}

    int main() {
        Docket *docket = Docket_new();
    
        struct sockaddr *remote_addr = ...;
        socklen_t remote_addr_len = ...;
    
        DocketEvent *event = DocketEvent_connect(docket, -1, remote_addr, remote_addr_len);
        // DocketEvent *event = DocketEvent_connect_hostname(docket, -1, "www.baidu.com", 80);
        DocketEvent_set_cb(event, on_read, on_write, on_event, NULL);
    
        Docket_loop(docket);
    }

# TODO
+ Timer