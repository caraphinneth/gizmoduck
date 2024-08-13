<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="ru_RU">
<context>
    <name>DebugTab</name>
    <message>
        <location filename="debug_tab.cpp" line="9"/>
        <source>Debug messages:</source>
        <translation>Отладочные сообщения:</translation>
    </message>
    <message>
        <location filename="debug_tab.cpp" line="11"/>
        <source>Tab groups:</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>FileDialog</name>
    <message>
        <location filename="file_dialog.cpp" line="15"/>
        <source>Preview</source>
        <translation>Эскиз</translation>
    </message>
</context>
<context>
    <name>MainWindow</name>
    <message>
        <location filename="browser_mainwindow.cpp" line="48"/>
        <source>Navigation</source>
        <translation>Навигация</translation>
    </message>
    <message>
        <location filename="browser_mainwindow.cpp" line="69"/>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
    <message>
        <location filename="browser_mainwindow.cpp" line="107"/>
        <source>Find text: </source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="browser_mainwindow.cpp" line="232"/>
        <source>[Connecting...]</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="browser_mainwindow.cpp" line="268"/>
        <source>Fetching contacts...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="browser_mainwindow.cpp" line="271"/>
        <source>Add contact...</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="browser_mainwindow.cpp" line="351"/>
        <location filename="browser_mainwindow.cpp" line="466"/>
        <source>Chat</source>
        <translation>Чат</translation>
    </message>
    <message>
        <location filename="browser_mainwindow.cpp" line="529"/>
        <source>Paste a full tox ID (NOT just public key) to add contact:</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Toggle proxy</source>
        <translation type="vanished">Прокси вкл/выкл</translation>
    </message>
    <message>
        <location filename="browser_mainwindow.cpp" line="83"/>
        <source>Close the active tab</source>
        <translation>Закрыть вкладку</translation>
    </message>
</context>
<context>
    <name>SettingsTab</name>
    <message>
        <source>Process model</source>
        <translation type="vanished">Многопоточная модель</translation>
    </message>
    <message>
        <source>Isolated processes. Recommended for security.</source>
        <translation type="vanished">Изолированные вкладки. Максимально безопасная модель.</translation>
    </message>
    <message>
        <source>Process per site. This way, tabs of the same origin will share the same process. Recommended for memory conservation.</source>
        <translation type="vanished">Один процесс на сайт. Сокращает потребление памяти.</translation>
    </message>
    <message>
        <source>Single process mode. This mode is not officially supported by Qt WebEngine and not recommended.
Note that single process may still make use of several threads.</source>
        <translation type="vanished">Один процесс (не рекомендуется). Разработчики Qt WebEngine больше не поддерживают этот режим.
Обратите внимание, что один процесс по-прежнему может создавать несколько потоков.</translation>
    </message>
    <message>
        <source>Global Settings (applied on restart)</source>
        <translation type="vanished">Эти настройки требуют перезапуска</translation>
    </message>
    <message>
        <source>Force GPU acceleration. Note that in most cases web tabs will be accelerated anyway, but this may help with video decoding.
May as well have no real effect.</source>
        <translation type="vanished">Активировать все виды GPU-ускорения. Как правило, веб-страницы уже используют GPU, но эта опция может помочь с аппаратным декодированием видео (или нет).</translation>
    </message>
    <message>
        <source>Enabled hardware-accelerated GPU memory buffers. Note this will have no real effect on most systems despite being reported as enabled.</source>
        <translation type="vanished">Включить аппаратную поддержку буферов в памяти GPU. На многих системах это не даст никакого реального эффекта.</translation>
    </message>
    <message>
        <source>Enable GPU rasterization of web pages. This is usually faster, but may also introduce delayed response.
There&apos;s no best answer here.</source>
        <translation type="vanished">Включить отрисовку веб-страниц на GPU. Это может быть быстрее, но также может замедлить отклик. Рекомендуется испытать оба режима.</translation>
    </message>
    <message>
        <source>Enable faster zero-copy implementation for CPU-rasterized pages.
Ideally, this will allow the rasterizer to switch between GPU and zero-copy implementation based on the content.
This is currently in development, typical problems you may face: rendering crash when switching to console and back; black view if the option above was not enabled.</source>
        <translation type="vanished">Включить аглоритм zero-copy для страниц, отрисованных на CPU.
В идеале, отрисовка будет автоматически переключаться между GPU и zero-copy, где это выгоднее.
На практике, на данном этапе возможны сбои при переключении в консоль и черный экран, если вы не включили опцию выше.</translation>
    </message>
    <message>
        <source>Allow lowres tiling when rasterizing on CPU. This increases CPU load when scrolling.
Disabling may cause visual artifacts when scrolling fast.</source>
        <translation type="vanished">Разрешить крупный тайлинг при отрисовке на CPU. Это увеличит загрузку процессора при прокрутке.
Отключение этой опции может привести к небольшим артефактам при прокрутке.</translation>
    </message>
    <message>
        <source>Enable TCP Fast Open, this is generally safe and results in a faster response.</source>
        <translation type="vanished">Включить TCP Fast Open. Эта опция (как правило) безопасна и способствует быстрому отклику.</translation>
    </message>
    <message>
        <source>Interface</source>
        <translation type="vanished">Интерфейс</translation>
    </message>
    <message>
        <source>Mouse wheel scroll speed:</source>
        <translation type="vanished">Скорость прокрутки колесиком мыши:</translation>
    </message>
    <message>
        <source>Proxy server settings (applied immediately for all tabs)</source>
        <translation type="vanished">Настройки прокси (общие для всех вкладок)</translation>
    </message>
    <message>
        <source>Default settings assume a Tor service running on localhost.</source>
        <translation type="vanished">Настройки по умолчанию рассчитаны на запущенный сервис Tor.</translation>
    </message>
    <message>
        <source>HTTP proxy</source>
        <translation type="vanished">HTTP прокси</translation>
    </message>
    <message>
        <source>SOCKS5 proxy</source>
        <translation type="vanished">SOCKS5 прокси</translation>
    </message>
    <message>
        <source>Host: </source>
        <translation type="vanished">Хост: </translation>
    </message>
    <message>
        <source>Port: </source>
        <translation type="vanished">Порт: </translation>
    </message>
    <message>
        <source>Process model (applied on restart)</source>
        <translation type="vanished">Многопоточная модель (требует перезапуска)</translation>
    </message>
    <message>
        <source>Web settings (applied on restart)</source>
        <translation type="vanished">Сетевые настройки (требуют перезапуска)</translation>
    </message>
    <message>
        <source>Enable Checker Imaging feature, which is supposed to improve loading speeds. Your mileage may vary.</source>
        <translation type="vanished">Включить Checker Imaging. Это может ускорить загрузку страниц с большим количеством изображений (или нет).</translation>
    </message>
    <message>
        <source>Rasterizing (applied on restart)</source>
        <translation type="vanished">Отрисовка (требует перезапуска)</translation>
    </message>
    <message>
        <source>Expert settings (applied on restart)</source>
        <translation type="vanished">Настройки для сисадминов (требуют перезапуска)</translation>
    </message>
    <message>
        <location filename="settings_tab.cpp" line="57"/>
        <source>Messenger</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <location filename="settings_tab.cpp" line="83"/>
        <source>Select File</source>
        <translation type="unfinished">Выбрать файл</translation>
    </message>
    <message>
        <location filename="settings_tab.cpp" line="83"/>
        <source>All Files (*.*)</source>
        <translation type="unfinished">Все файлы (*.*)</translation>
    </message>
    <message>
        <location filename="settings_tab.cpp" line="114"/>
        <source>Content Filters:</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>TabWidget</name>
    <message>
        <source>(Untitled)</source>
        <translation type="vanished">Новая вкладка</translation>
    </message>
    <message>
        <source>Untitled</source>
        <translation type="vanished">Новая вкладка</translation>
    </message>
    <message>
        <source>Save as...</source>
        <translation type="vanished">Сохранить как...</translation>
    </message>
    <message>
        <location filename="tab_manager.cpp" line="968"/>
        <source>Debug</source>
        <translation>Отладка</translation>
    </message>
    <message>
        <location filename="tab_manager.cpp" line="934"/>
        <source>Save as</source>
        <translation>Сохранить как...</translation>
    </message>
    <message>
        <location filename="tab_manager.cpp" line="947"/>
        <source>Settings</source>
        <translation>Настройки</translation>
    </message>
</context>
<context>
    <name>ToxManager</name>
    <message>
        <location filename="tox_ui.cpp" line="335"/>
        <source>Friend request.</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>ToxWidget</name>
    <message>
        <location filename="tox_ui.cpp" line="404"/>
        <source>Chat with </source>
        <translation>Чат с </translation>
    </message>
    <message>
        <location filename="tox_ui.cpp" line="436"/>
        <source>Select File</source>
        <translation type="unfinished">Выбрать файл</translation>
    </message>
    <message>
        <location filename="tox_ui.cpp" line="436"/>
        <source>All Files (*.*)</source>
        <translation type="unfinished">Все файлы (*.*)</translation>
    </message>
    <message>
        <location filename="tox_ui.cpp" line="516"/>
        <source> is typing...</source>
        <translation type="unfinished"></translation>
    </message>
</context>
<context>
    <name>WebPage</name>
    <message>
        <location filename="webpage.cpp" line="86"/>
        <source>Select File</source>
        <translation>Выбрать файл</translation>
    </message>
    <message>
        <location filename="webpage.cpp" line="86"/>
        <source>All Files (*.*)</source>
        <translation>Все файлы (*.*)</translation>
    </message>
    <message>
        <location filename="webpage.cpp" line="110"/>
        <source>Enter username and password for &quot;%1&quot; at %2</source>
        <translation>Введите логин и пароль для &quot;%1&quot; на %2</translation>
    </message>
    <message>
        <source>All Files (*.*);;</source>
        <translation type="vanished">Все файлы (*.*)</translation>
    </message>
</context>
<context>
    <name>WebView</name>
    <message>
        <location filename="webview.cpp" line="124"/>
        <source>Open Link in This Tab</source>
        <translation>Открыть в текущей вкладке</translation>
    </message>
    <message>
        <location filename="webview.cpp" line="137"/>
        <source>Search &quot;</source>
        <translation>Найти &quot;</translation>
    </message>
    <message>
        <location filename="webview.cpp" line="137"/>
        <source>&quot; on the web</source>
        <translation>&quot; в интернете</translation>
    </message>
    <message>
        <location filename="webview.cpp" line="145"/>
        <source>Follow &quot;</source>
        <translation>Перейти по ссылке &quot;</translation>
    </message>
    <message>
        <location filename="webview.cpp" line="155"/>
        <source>Translate Page</source>
        <translation>Перевести страницу</translation>
    </message>
    <message>
        <location filename="webview.cpp" line="160"/>
        <source>Run yt-dlp</source>
        <translation type="unfinished"></translation>
    </message>
    <message>
        <source>Search </source>
        <translation type="vanished">Найти </translation>
    </message>
    <message>
        <source> on the web</source>
        <translation type="vanished"> в интернете</translation>
    </message>
</context>
</TS>
