<?php
//同時に接続できるクライアントは1台だけ.
//2台目が接続すると1台目とサーバの接続が切れるまで待機することになる
$socketHole = "tcp://localhost:11180";
//ソケットはファイルディスクプリタ。だからreadとwriteができる
//ファイルディスクプリタ作成
$socket = stream_socket_server($socketHole);
if ($socket === false) {
    print("ソケットの作成に失敗しました。");
    exit();
} else {
    while(true) {
        // 何らかのクライアント側からこのサーバーまで
        // 接続されるために待ち受けを行う
        $stream = stream_socket_accept($socket, -1);
        if ($stream !== false) {
            // ソケットの確立が成功した場合
            $clientName = stream_socket_get_name($stream, true);  //・・・・・(1)
            print($clientName."が接続しました".PHP_EOL);
            $successMessage = "[$clientName]:I have solved a connection from your client:".date("Y-m-d H:i:s").PHP_EOL;
            fwrite($stream, $successMessage, strlen($successMessage));
            while(true) {
                $temp = fread($stream, 1024);
                // サーバー側のコンソールにログとして出力する
                $log = "[$clientName]:サーバー側が[$temp]というメッセージを受信".date("Y-m-d H:i:s").PHP_EOL;
                print($log);
                fwrite($stream, $log, strlen($log));
                if (strlen($temp) === 0) {
                    break;
                }
            }
        }
    }
}
fclose($socket);