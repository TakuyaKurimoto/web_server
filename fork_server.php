<?php
//参考
//https://ryo511.info/archives/3901
$server = stream_socket_server('tcp://127.0.0.1:'.(getenv('PORT') ?: 11180), $errno, $errstr);

if (false === $server) {
    fwrite(STDERR, "Failed creating socket server: $errstr\n");
    exit(1);
}

echo "Waiting…\n";

const MAX_PROCS = 2;
$children = [];

for (;;) {
    $read = [$server];
    $write = null;
    $except = null;

    stream_select($read, $write, $except, 0, 500);

    foreach ($read as $stream) {
        
        if (count($children) < MAX_PROCS) {
            $conn = @stream_socket_accept($server, -1, $peer);

            if (!is_resource($conn)) {
                continue;
            }

            echo "Starting a new child process for $peer\n";

            $pid = pcntl_fork();

            if ($pid > 0) {
                $children[] = $pid;
                //closeしても子プロセスではcloseされないから大丈夫。
                fclose($conn);
            } elseif ($pid === 0) {
                // Child process, implement our echo server
                $childPid = posix_getpid();
                fwrite($conn, "You are connected to process $childPid\n");

                
                
                fclose($conn);

            }
        }
    }

    // Do housekeeping on exited childs
    foreach ($children as $i => $child) {
        //WNOHANGを指定すると、子プロセスが終了していない場合もブロックされない
        $result = pcntl_waitpid($child, $status, WNOHANG);
        //正常終了か
        if ($result > 0 && pcntl_wifexited($status)) {
            unset($children[$i]);
        }
    }

    echo "\t".count($children)." connected\r";
}