<?php

$socketHole = "tcp://localhost:11180";
$errorNunmber = null;
$errorMessage = null;
$socket = stream_socket_server($socketHole, $errorNunmber, $errorMessage);
if ($socket === false) {
    print("ソケットの確立に失敗しました。");
    exit();
} else {
    // このソケットサーバーに接続してきた全クライアントを
    // 監視用の配列に確保する
    $clients = array();
    while(true) {
        
        // 何らかのクライアント側からこのサーバーまで
        // 接続されるために待ち受けを行う(※これは初回接続時のみ)
        // @をつけるとwarningが出なくなる
        $stream = @stream_socket_accept($socket, 0);
        if ($stream !== false) {
            // クライアントの初回接続時のみ
            $clientName = stream_socket_get_name($stream, true);
            print($clientName."がサーバーに接続しました".PHP_EOL);
            
            // ストリーム監視用の配列に確保
            $clients[$clientName] = $stream;
        }
        // 本ソケットサーバー側に接続している
        // クライアントが0の状態を予防
        if (count($clients) === 0) {
            continue;
        }
        // (1) 同時接続されたソケットを集中管理できる処理を書く
        $read = $write = $error = $clients;
        //stream_selectの前と後で$readの値が変化するので注意
        $number = stream_select($read, $write, $error, 3);
        
        if ($number > 0) {
            
            foreach ($read as $key => $fp) {
                //接続が切れている
                if (!$fp){
                    unset($clients[$key]);
                    continue;
                }
                $temp = fread($fp, 1024);
                if (strlen($temp) === 0) {
                    continue;
                }
                // サーバー側のコンソールにログとして出力する
                $readClientName = stream_socket_get_name($fp, true);
                $serverLog = "[$readClientName]:サーバー側が[$temp]というメッセージを受信".date("Y-m-d H:i:s").PHP_EOL;
                print($serverLog);
                foreach ($clients as $innerKey => $innerFp) {
                    //接続が切れている
                    if(!stream_socket_get_name($innerFp, true)){
                        unset($clients[$innerKey]);
                        continue;
                    }

                    //参考　https://zenn.dev/bigen1925/books/introduction-to-web-application-with-python/viewer/improve-to-minimal-web-server#keep-alive
                    $body  = "<html><body><h1>It works!</h1></body></html>";
                    $header =          "HTTP/1.1 200 OK
                                        Date: " . date(DATE_RFC2822) .
                                       "Server: hogehoge/1.0 (Unix)
                                        TCN: choice
                                        Last-Modified: Thu, 29 Aug 2019 05:05:59 GMT 
                                        Content-Length:" . strlen($body) .                                       
                                       "Connection: Close
                                        Content-Type: text/html\r\n";

                    
                    fwrite($clients[$innerKey], $header ."\r\n" . $body, strlen($header . $body));
                    fclose($clients[$innerKey]);
                    unset($clients[$innerKey]);
                }
            }
        }
    }
}
fclose($socket);