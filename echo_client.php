<?php

$destination = "tcp://localhost:11180";
$socket = stream_socket_client($destination);

if ($socket ===false) {
    print("ソケットの確立に失敗");
    exit();
} else {
    // ソケットへの接続時お名前を入力する
    //yournameに入力が格納される
    $yourName = readline(">>> Input your name ");
    stream_set_timeout($socket, 1);
    while (true) {
        print("{$yourName}さん");
        $input = readline(">>> ");
        $sendingMessage = "{$yourName}:{$input}";
        fwrite($socket, $sendingMessage, strlen($sendingMessage));
        
        // サーバー側からのレスポンスを取得
        while(true) {
            $read = fread($socket, 1024);
            print(1);
            /*
            $info = stream_get_meta_data($socket);
            if ($info['timed_out']) {
                echo 'Connection timed out!';
                exit();
            }  
            */
            print($read.PHP_EOL);
            
            if (strlen($read) === 0) {
                break;
            }
        }
    }
}