<?php
    //参考　https://qiita.com/mashed_potatoes/items/95dfa711d3330ac83c35
    $destination = "tcp://yahoo.co.jp:80";
    $socket = stream_socket_client($destination);
    if ($socket ===false) {
        print("ソケットの確立に失敗");
        exit();
    } else {
        $message = "GET / HTTP/1.1\r\nHost: hogehoge\r\n";
        $message .= "\r\n";
        fwrite($socket, $message, strlen($message));
        //(1)ソケットの書き込みに対するタイムアウトを記述する
        stream_set_timeout($socket, 1);
        while(true) {
            //タイムアアウトもしくは1024バイト読むまで待機
            $read = fread($socket, 1024);
            
            if (strlen($read) === 0) {
                break;
            }
            print($read);
        }
    }