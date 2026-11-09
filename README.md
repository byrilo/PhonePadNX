# PhonePadNX

Телефон как дополнительный геймпад для Nintendo Switch с Atmosphère или Ultra NX.
На телефоне нужен только браузер. Switch и телефон должны быть в одной Wi-Fi сети.

## Установка

1. Скачай ZIP из [Releases](https://github.com/byrilo/PhonePadNX/releases).
2. Распакуй его в корень SD-карты и перезагрузи Switch.
3. Открой PhonePadNX в Homebrew Menu.
4. Отсканируй QR-код телефоном и нажми «Подключиться».
5. Закрой PhonePadNX кнопкой `+` и запусти игру.
6. Если нужно выбрать игрока: «Контроллеры → Изменить способ удерживания/порядок», затем L + R на телефоне.

## Что работает

- Кнопки Pro Controller и два стика.
- Несколько касаний одновременно.
- Подключение по PIN, один телефон за раз.
- Сброс нажатий при потере связи.

Первая тестовая версия. На настоящем Switch ещё не проверена.
Гироскоп, вибрация и несколько телефонов пока не поддерживаются.
Сервер работает по HTTP на порту 8080: используй домашнюю сеть, без проброса порта в интернет.

## Удаление

Выключи консоль и удали с SD папки:

- `atmosphere/contents/4200000000505044`
- `switch/PhonePadNX`

## Сборка

Нужны devkitPro с `switch-dev`, make и Python 3.

```sh
python3 tools/embed.py
make -C sysmodule -j2
make -C launcher -j2
python3 tools/package.py
```

Архив для SD появится в `dist/`. GitHub Actions собирает его автоматически.

## Автор

[byrilo](https://github.com/byrilo)

Генератор QR: [Project Nayuki](https://github.com/nayuki/QR-Code-generator), MIT.
Шаблоны сборки: [switchbrew](https://github.com/switchbrew/switch-examples).
