# PhonePadNX

Телефон как дополнительный геймпад для Switch. Версия 0.3.3 предназначена для проверки на консоли. Причина перезагрузок на версиях 0.1.x не подтверждена.

1. Распакуй ZIP в корень SD и перезагрузи консоль.
2. Открой PhonePadNX, нажми A и отсканируй QR телефоном в той же Wi-Fi сети.
3. Подключись, выйди из PhonePadNX кнопкой + и запусти игру. Для выбора игрока нажми L + R на телефоне в меню порядка контроллеров.
4. Для остановки открой PhonePadNX и нажми X. Сервер также останавливается через 30 минут.

При повторной перезагрузке или сбое стика выключи консоль и удали только папку модуля `atmosphere/contents/4200000000505044`. Диагностическая версия 0.2.0 остаётся доступной отдельно.

Сборка через devkitPro: `python3 tools/embed.py`, `make -C sysmodule`, `make -C launcher`, `python3 tools/package.py`.

Автор: [byrilo](https://github.com/byrilo). QR: [Project Nayuki](https://github.com/nayuki/QR-Code-generator), MIT. Шаблоны: [switchbrew](https://github.com/switchbrew/switch-examples).
