# EShot 📸

**EShot**, Windows için geliştirilmiş, **Lightshot** ve **Flameshot** benzeri, modern, hızlı ve hafif bir ekran görüntüsü alma aracıdır. C++, Qt 6 ve Windows Native API kullanılarak geliştirilmiştir.

![EShot Banner](/resources/icons/pen.svg) 
*(Buraya daha sonra uygulamanın ekran görüntüsünü ekleyebilirsiniz)*

## ✨ Özellikler

*   **⚡ Hızlı ve Hafif:** Minimum sistem kaynağı kullanımı.
*   **🖊️ Gelişmiş Çizim Araçları:** Kalem, Ok, Dikdörtgen, **Çember**, Vurgulayıcı, Bulanıklaştırma (Blur) ve **Sayaç (#)**.
*   **🎨 Modern Arayüz:** Şık tasarım, animasyonlar ve **Koyu Tema** desteği.
*   **📌 Pin to Desktop:** Seçilen alanı ekrana sabitleme (Always-on-top pencere).
*   **🖱️ Kolay Seçim:** `Ctrl` + Sürükle ile seçim alanını taşıma.
*   **💾 Otomatik Kayıt:** Özelleştirilebilir dosya adı şablonları (`%Y-%m-%d` vb.) ve kayıt yolu.
*   **📋 Pano Entegrasyonu:** Kayıttan sonra otomatik kopyalama, dosya yolunu kopyalama seçenekleri.
*   **🖥️ Multi-Monitor DPI Desteği:** Windows Native API (`BitBlt`) ile piksel-piksel hatasız yakalama.

## 🛠️ Kurulum ve Derleme (Build)

Bu projeyi geliştirmek veya derlemek için aşağıdaki araçlara ihtiyacınız vardır:

### Gereksinimler
*   **CMake** (3.16 veya üzeri)
*   **Qt 6.x** (Core, Gui, Widgets modülleri)
*   **Visual Studio Build Tools 2022** (veya uyumlu MSVC derleyicisi)

### Derleme Adımları

1.  Repoyu klonlayın:
    ```bash
    git clone https://github.com/kullaniciadi/EShot.git
    cd EShot
    ```

2.  Build klasörü oluşturun ve CMake'i çalıştırın:
    ```bash
    mkdir build
    cd build
    cmake ..
    ```

3.  Projeyi derleyin (Release modunda):
    ```bash
    cmake --build . --config Release
    ```

4.  Çalıştırılabilir dosya `build/bin/Release/EShot.exe` konumunda olacaktır.

## 🚀 Kullanım

*   **PrtSc (Print Screen):** Ekran görüntüsü alma modunu başlatır.
*   **Sol Tık + Sürükle:** Alan seçer.
*   **Araç Çubuğu:** Seçimden sonra araçlar belirir (Kalem, Ok, vb.).
*   **Ayarlar:** Tepsi simgesine (System Tray) sağ tıklayarak ulaşılabilir.
*   **Pinleme:** Araç çubuğundaki "İğne" ikonuna tıklayarak seçimi ekrana sabitleyin.

### Kısayollar
| Tuş | İşlev |
| :--- | :--- |
| `PrtSc` | Yakalamayı Başlat |
| `Enter` / `Ctrl+C` | Seçimi Kopyala |
| `Ctrl+S` | Kaydet |
| `Esc` | İptal Et / Kapat |
| `Ctrl+Z` | Geri Al (Undo) |
| `Ctrl+Y` | İleri Al (Redo) |
| `Shift` (Çizerken) | Tam Kare / Tam Daire Çiz |
| `Ctrl` + Sürükle | Seçim Alanını Taşı |

## 🤝 Katkıda Bulunma

Pull request'ler kabul edilir. Büyük değişiklikler için önce lütfen tartışmak üzere bir konu (issue) açınız.

## 📜 Lisans

Bu proje [MIT](LICENSE) lisansı altında lisanslanmıştır.
