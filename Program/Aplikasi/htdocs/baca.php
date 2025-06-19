<?php
// Nama file yang akan dibaca
$namaFile = "pesan.txt";

// Memeriksa apakah file dengan nama yang ditentukan ada
if (file_exists($namaFile)) {
    // Jika file ditemukan, membaca isi file
    $pesan = file_get_contents($namaFile);
    // Menampilkan isi file sebagai output
    echo $pesan;
} else {
    // Jika file tidak ditemukan, menampilkan pesan error
    echo "File $namaFile tidak ditemukan.";
}
?>
