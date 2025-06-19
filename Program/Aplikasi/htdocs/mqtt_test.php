<?php

// Memuat autoload dari Composer untuk mengimpor pustaka yang diperlukan
require 'vendor/autoload.php';

// Menggunakan namespace untuk kelas-kelas dari pustaka PhpMqtt\Client
use PhpMqtt\Client\MqttClient;
use PhpMqtt\Client\ConnectionSettings;

try {
    // Konfigurasi broker MQTT
    $host = '123.231.250.36'; // Ganti dengan host broker MQTT Anda
    $port = 21283;            // Port broker MQTT (default biasanya 1883 atau 8883 untuk SSL)
    $clientId = 'php-mqtt-client'; // ID unik untuk klien MQTT
    $topic = 'terima';    // Topik yang akan menerima pesan
    $message = 'MQTT ON'; // Pesan yang akan dikirim ke broker MQTT

    // Username dan password untuk autentikasi ke broker MQTT
    $username = 'tester1'; // Ganti dengan username broker MQTT
    $password = 'itbstikombali11'; // Ganti dengan password broker MQTT

    // Membuat objek klien MQTT dengan parameter host, port, dan ID klien
    $mqtt = new MqttClient($host, $port, $clientId);

    // Mengatur parameter koneksi ke broker MQTT
    $connectionSettings = (new ConnectionSettings())
        ->setUsername($username)    // Mengatur username autentikasi
        ->setPassword($password)    // Mengatur password autentikasi
        ->setKeepAliveInterval(60)  // Mengatur interval keep-alive agar koneksi tetap hidup
        ->setLastWillTopic('test/lastwill')  // Mengatur topik "last will" jika klien terputus tiba-tiba
        ->setLastWillMessage('Client disconnected unexpectedly') // Pesan "last will" yang akan dikirim jika klien terputus
        ->setLastWillQualityOfService(1); // Mengatur QoS untuk pesan "last will" (1 = setidaknya sekali)

    // Menghubungkan klien ke broker MQTT menggunakan pengaturan yang sudah dibuat
    $mqtt->connect($connectionSettings, true);

    // Berlangganan ke topik 'satu' dan menangani pesan yang diterima
    $mqtt->subscribe('satu', function ($topic, $message) {
        // Menampilkan pesan yang diterima di terminal
        printf("Received message on topic [%s]: %s\n", $topic, $message);

        // Menyimpan pesan ke dalam file 'pesan.txt'
        file_put_contents("pesan.txt", $message);
    }, 0); // QoS 0 berarti pesan diterima dengan usaha terbaik tanpa jaminan pengiriman ulang

    // Mengirim pesan ke topik yang ditentukan
    $mqtt->publish("terima", $message, MqttClient::QOS_AT_LEAST_ONCE); // QoS 1 berarti pesan dikirim setidaknya sekali
    echo "Pesan '{$message}' berhasil dikirim ke topik '{$topic}'.\n";

    // Menjalankan loop untuk mendengarkan pesan masuk dari broker MQTT
    $mqtt->loop(); // Loop ini akan berjalan terus untuk menangani pesan masuk

    // Memutuskan koneksi dari broker MQTT (kode ini tidak akan dijalankan karena loop berjalan terus)
    $mqtt->disconnect();

} catch (Exception $e) {
    // Menangkap dan menampilkan kesalahan jika terjadi masalah
    echo 'Error: ' . $e->getMessage() . "\n";
}