/* ==========================================================================
   config_editor.js — パラメータ設定ファイル(CSV)をその場で編集する共通部品
   --------------------------------------------------------------------------
   マッピングの実行画面から設定を書き換えられるようにする。トピック名と座標系は
   選択中の ROS Bag に実際に記録されているものをプルダウンの候補として出すので、
   利用者が正しい名前を調べて手入力する必要がない。

   マッピングの bash スクリプトは CSV を行番号で読むため、行の追加・削除・
   並べ替えは行わず「指定値」の列だけを書き換える（保存は server.py 側で実施）。

   使い方:
     const editor = ConfigEditor.create({
         selectElement:  document.getElementById('config-file'),   // 設定ファイル選択
         configFile:     'hokuyo_slam_topics_cfg.csv',  // selectElement が無いときの編集対象
         bodyElement:    document.getElementById('config-editor-body'),
         messageElement: document.getElementById('config-save-message'),
         bagPath:        '/path/to/rosbag',   // 省略可（候補が減るだけ）
         saveLabel:      '保存して再チェック', // 省略可（既定は「保存」）
         onSaved:        function (savedName) { ... }   // 省略可
     });
     editor.reload();
   ========================================================================== */
window.ConfigEditor = (function () {
    'use strict';

    // 「直接入力」を選んだときの目印。トピック名として使われない値にしておく。
    const CUSTOM_VALUE = '__custom__';

    function create(config) {
        // selectElement は「設定ファイルを選ぶプルダウン」。
        // ファイル管理画面のように編集対象が1つに決まっている場合は
        // selectElement を省略し、configFile でファイル名を直接渡す。
        const selectElement = config.selectElement;
        const bodyElement = config.bodyElement;
        const messageElement = config.messageElement;
        const onSaved = config.onSaved;
        // 保存ボタンの文言。設定チェックのある画面では「保存して再チェック」にする。
        const saveLabel = config.saveLabel || '保存';

        // 候補の取得元。ROS Bag を後から選び直せるように let で持つ。
        let bagPath = config.bagPath || '';
        let currentFile = config.configFile
            || (selectElement ? selectElement.value : '');

        // 設定ファイルの1項目分の入力欄を組み立てる。
        // 候補があるものはプルダウン、無いものは文字入力にする。
        function buildControl(row) {
            const wrapper = document.createElement('div');

            const input = document.createElement('input');
            input.type = 'text';
            input.className = 'config-table__input';
            input.dataset.option = row.name;
            input.value = row.value;

            if (!row.choices || row.choices.length === 0) {
                wrapper.appendChild(input);
                return wrapper;
            }

            const select = document.createElement('select');
            select.className = 'config-table__select';
            select.dataset.option = row.name;

            // 今の値が候補に無い場合は先頭に出しておく。
            // トピック名と座標系については、ROS Bagに無いことが赤字で分かるようにする。
            const known = row.choices.some(choice => choice.value === row.value);
            if (row.value && !known) {
                const missing = document.createElement('option');
                missing.value = row.value;
                const isFromBag = (row.kind === 'topic' || row.kind === 'frame');
                missing.textContent = isFromBag
                    ? row.value + '（このROS Bagにはありません）'
                    : row.value;
                missing.className = isFromBag
                    ? 'config-option--missing'
                    : 'config-option--plain';
                select.appendChild(missing);
            }

            row.choices.forEach(choice => {
                const option = document.createElement('option');
                option.value = choice.value;
                option.textContent = (choice.recommended ? '★ ' : '') + choice.label;
                // 型が合っている・座標系が一致しているものは緑字にする。
                option.className = choice.recommended
                    ? 'config-option--match'
                    : 'config-option--plain';
                select.appendChild(option);
            });

            const custom = document.createElement('option');
            custom.value = CUSTOM_VALUE;
            custom.textContent = '（直接入力）';
            custom.className = 'config-option--plain';
            select.appendChild(custom);

            select.value = row.value;
            input.hidden = true;

            // 閉じている状態のプルダウンにも、選択中の項目と同じ色を反映させる。
            function applySelectedColor() {
                const selected = select.selectedOptions[0];
                select.classList.remove('is-match', 'is-missing');
                if (!selected) { return; }
                if (selected.classList.contains('config-option--match')) {
                    select.classList.add('is-match');
                } else if (selected.classList.contains('config-option--missing')) {
                    select.classList.add('is-missing');
                }
            }
            applySelectedColor();

            select.addEventListener('change', () => {
                const isCustom = select.value === CUSTOM_VALUE;
                input.hidden = !isCustom;
                applySelectedColor();
                if (isCustom) {
                    input.focus();
                }
            });

            wrapper.appendChild(select);
            wrapper.appendChild(input);
            return wrapper;
        }

        // 編集欄に入力されている値を {オプション名: 値} で取り出す。
        function collectValues() {
            const values = {};
            bodyElement.querySelectorAll('.config-table__select').forEach(select => {
                if (select.value !== CUSTOM_VALUE) {
                    values[select.dataset.option] = select.value;
                }
            });
            bodyElement.querySelectorAll('.config-table__input').forEach(input => {
                if (!input.hidden) {
                    values[input.dataset.option] = input.value.trim();
                }
            });
            return values;
        }

        // 設定ファイルの中身を読み込んで編集欄を作る。
        function reload() {
            if (!bodyElement) { return; }
            if (selectElement) {
                currentFile = selectElement.value;
            }
            const configFile = currentFile;
            if (!configFile) {
                bodyElement.innerHTML =
                    '<p class="text-muted">編集するには、上で設定ファイルを選択してください。</p>';
                return;
            }

            bodyElement.innerHTML = '<p class="text-muted">読み込んでいます…</p>';
            fetch('/mapping_config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ config_file: configFile, input_bag_path: bagPath })
            })
                .then(response => response.json())
                .then(data => {
                    if (data.status !== 'success') {
                        bodyElement.innerHTML = '';
                        const message = document.createElement('p');
                        message.className = 'text-error';
                        message.textContent = data.message || '設定ファイルを読み込めませんでした。';
                        bodyElement.appendChild(message);
                        return;
                    }
                    render(data);
                })
                .catch(error => {
                    console.error('設定ファイルの読み込みに失敗しました:', error);
                    bodyElement.innerHTML =
                        '<p class="text-error">サーバーと通信できませんでした。</p>';
                });
        }

        function render(data) {
            bodyElement.innerHTML = '';

            if (!data.has_bag_info) {
                const warning = document.createElement('p');
                warning.className = 'note';
                warning.textContent =
                    'ROS Bag の内容を読み取れなかったため、トピック名の候補は表示されません。';
                bodyElement.appendChild(warning);
            }

            const wrap = document.createElement('div');
            wrap.className = 'config-table-wrap';
            const table = document.createElement('table');
            table.className = 'table config-table';

            const thead = document.createElement('thead');
            thead.innerHTML = '<tr><th>項目</th><th>指定値</th><th>デフォルト値</th></tr>';
            table.appendChild(thead);

            const tbody = document.createElement('tbody');
            data.rows.forEach(row => {
                const tr = document.createElement('tr');

                const nameCell = document.createElement('td');
                const label = document.createElement('div');
                label.textContent = row.label;
                const rawName = document.createElement('div');
                rawName.className = 'config-table__name';
                rawName.textContent = row.name;
                nameCell.appendChild(label);
                nameCell.appendChild(rawName);
                tr.appendChild(nameCell);

                const valueCell = document.createElement('td');
                valueCell.appendChild(buildControl(row));
                tr.appendChild(valueCell);

                const defaultCell = document.createElement('td');
                defaultCell.className = 'config-table__default';
                defaultCell.textContent = row.default || '-';
                tr.appendChild(defaultCell);

                tbody.appendChild(tr);
            });
            table.appendChild(tbody);
            wrap.appendChild(table);
            bodyElement.appendChild(wrap);

            const nameField = document.createElement('div');
            nameField.className = 'field';
            nameField.innerHTML =
                '<label for="config-save-name">保存するファイル名:</label>'
                + '<input type="text" id="config-save-name">';
            bodyElement.appendChild(nameField);
            const saveNameInput = nameField.querySelector('#config-save-name');
            saveNameInput.value = data.config_name;

            const actions = document.createElement('div');
            actions.className = 'actions';
            const saveButton = document.createElement('button');
            saveButton.type = 'button';
            saveButton.className = 'btn btn--primary';
            saveButton.textContent = saveLabel;
            const revertButton = document.createElement('button');
            revertButton.type = 'button';
            revertButton.className = 'btn btn--secondary';
            revertButton.textContent = '編集を取り消す';
            actions.appendChild(saveButton);
            actions.appendChild(revertButton);
            bodyElement.appendChild(actions);

            revertButton.addEventListener('click', () => {
                setMessage('', 'note');
                reload();
            });
            saveButton.addEventListener('click', () => {
                save(data.config_name, saveNameInput.value.trim(), saveButton);
            });
        }

        function setMessage(text, className) {
            if (!messageElement) { return; }
            messageElement.className = className;
            messageElement.textContent = text;
        }

        function save(sourceName, saveAs, saveButton) {
            if (!saveAs) {
                setMessage('保存するファイル名を入力してください。', 'text-error');
                return;
            }
            saveButton.disabled = true;
            setMessage('保存しています…', 'note');

            fetch('/save_mapping_config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    config_file: sourceName,
                    save_as: saveAs,
                    values: collectValues()
                })
            })
                .then(response => response.json())
                .then(data => {
                    saveButton.disabled = false;
                    if (data.status !== 'success') {
                        setMessage(data.message || '保存に失敗しました。', 'text-error');
                        return;
                    }
                    setMessage(data.message, 'text-success');
                    // 別名で保存した場合は、以後その新しいファイルを編集対象にする。
                    currentFile = data.config_file;
                    updateSelectOptions(data.config_files, data.config_file);
                    reload();
                    // 呼び出し元に保存を伝える（設定チェックのやり直しなど）。
                    if (typeof onSaved === 'function') {
                        onSaved(data.config_file, data.config_files);
                    }
                })
                .catch(error => {
                    console.error('設定ファイルの保存に失敗しました:', error);
                    saveButton.disabled = false;
                    setMessage('サーバーと通信できませんでした。', 'text-error');
                });
        }

        // 別名で保存した場合に備えて、設定ファイルのプルダウンを作り直す。
        function updateSelectOptions(configFiles, selected) {
            if (!selectElement || !configFiles) { return; }
            selectElement.innerHTML = '';
            const placeholder = document.createElement('option');
            placeholder.value = '';
            placeholder.textContent = '-- 設定ファイルを選択 (任意) --';
            selectElement.appendChild(placeholder);
            configFiles.forEach(name => {
                const option = document.createElement('option');
                option.value = name;
                option.textContent = name;
                selectElement.appendChild(option);
            });
            selectElement.value = selected;
        }

        // 候補の取得元にする ROS Bag を切り替える。
        function setBagPath(newBagPath) {
            bagPath = newBagPath || '';
        }

        return {
            reload: reload,
            setMessage: setMessage,
            setBagPath: setBagPath,
            collectValues: collectValues
        };
    }

    return { create: create };
}());
