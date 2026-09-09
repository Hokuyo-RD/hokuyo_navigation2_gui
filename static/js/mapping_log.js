/* ==========================================================================
   mapping_log.js — 実行ログの表示と、指摘（設定チェック／エラー）の描画
   --------------------------------------------------------------------------
   マッピングやPCD変換は別ターミナルで動くため、失敗しても画面には
   「処理を続行中です…」としか出ない。ここでは server.py の /mapping_log を
   一定間隔で読み、生ログと「非エンジニア向けに翻訳したエラー説明」を出す。

   使い方:
     const viewer = MappingLog.createViewer({
         logElement:    document.getElementById('log-output'),
         issueElement:  document.getElementById('log-issues'),
         onFinished:    function (exitCode) { ... }   // 省略可
     });
     viewer.start(jobId);
     viewer.stop();
   ========================================================================== */
window.MappingLog = (function () {
    'use strict';

    var POLL_INTERVAL_MS = 2000;

    // 指摘の深刻度と、見出しに付ける記号・.alert の修飾子の対応。
    var LEVEL_STYLE = {
        error:   { modifier: 'alert--error',   icon: '⛔' },
        warning: { modifier: 'alert--warning', icon: '⚠️' },
        ok:      { modifier: 'alert--success', icon: '✅' },
        info:    { modifier: 'alert--info',    icon: 'ℹ️' }
    };

    /** 指摘1件を .alert 1枚として組み立てる。 */
    function buildIssue(issue) {
        var style = LEVEL_STYLE[issue.level] || LEVEL_STYLE.info;
        var box = document.createElement('div');
        box.className = 'issue alert ' + style.modifier;

        var title = document.createElement('p');
        title.className = 'issue__title';
        title.textContent = style.icon + ' ' + issue.title;
        box.appendChild(title);

        if (issue.detail) {
            var detail = document.createElement('p');
            detail.className = 'issue__detail';
            detail.textContent = issue.detail;
            box.appendChild(detail);
        }
        if (issue.hint) {
            var hint = document.createElement('p');
            hint.className = 'issue__hint';
            hint.textContent = issue.hint;
            box.appendChild(hint);
        }
        return box;
    }

    /**
     * 指摘の一覧を描画する。
     * 「確認できました」(ok) は件数が多くなるので、既定では折りたたむ。
     */
    function renderIssues(container, issues, options) {
        options = options || {};
        container.innerHTML = '';
        if (!issues || issues.length === 0) {
            if (options.emptyMessage) {
                var empty = document.createElement('p');
                empty.className = 'text-muted';
                empty.textContent = options.emptyMessage;
                container.appendChild(empty);
            }
            return;
        }

        var problems = issues.filter(function (issue) { return issue.level !== 'ok'; });
        var confirmed = issues.filter(function (issue) { return issue.level === 'ok'; });

        var list = document.createElement('div');
        list.className = 'issue-list';
        problems.forEach(function (issue) { list.appendChild(buildIssue(issue)); });
        container.appendChild(list);

        if (confirmed.length > 0) {
            var details = document.createElement('details');
            details.className = 'issue-collapse';
            var summary = document.createElement('summary');
            summary.textContent = '確認できた項目 (' + confirmed.length + '件)';
            details.appendChild(summary);
            var okList = document.createElement('div');
            okList.className = 'issue-list';
            confirmed.forEach(function (issue) { okList.appendChild(buildIssue(issue)); });
            details.appendChild(okList);
            container.appendChild(details);
        }
    }

    /**
     * 実行ログのビューアを作る。
     * logElement には生ログを、issueElement には翻訳した指摘を描画する。
     */
    function createViewer(config) {
        var logElement = config.logElement;
        var issueElement = config.issueElement;
        var onFinished = config.onFinished;

        var jobId = null;
        var offset = 0;
        var timer = null;
        var seenKeys = {};
        var issues = [];
        // 終了を検知したかどうか。onFinished を2回以上呼ばないための番人。
        // これが無いと onFinished から flush() を呼んだときに読み込みが止まらなくなる。
        var finished = false;

        function appendText(text) {
            if (!text || !logElement) { return; }
            // 一番下までスクロールしていたときだけ、追従スクロールする。
            var atBottom = logElement.scrollTop + logElement.clientHeight
                >= logElement.scrollHeight - 8;
            logElement.textContent += text;
            if (atBottom) {
                logElement.scrollTop = logElement.scrollHeight;
            }
        }

        function addEvents(events) {
            if (!events || events.length === 0 || !issueElement) { return; }
            var added = false;
            events.forEach(function (event) {
                if (seenKeys[event.key]) { return; }
                seenKeys[event.key] = true;
                issues.push(event);
                added = true;
            });
            if (added) {
                renderIssues(issueElement, issues);
            }
        }

        function poll() {
            if (!jobId) { return; }
            fetch('/mapping_log?job_id=' + encodeURIComponent(jobId) + '&offset=' + offset)
                .then(function (response) { return response.json(); })
                .then(function (data) {
                    if (data.status !== 'success') { return; }
                    offset = data.offset;
                    appendText(data.text);
                    addEvents(data.events);
                    if (data.finished && !finished) {
                        finished = true;
                        stop();
                        if (typeof onFinished === 'function') {
                            onFinished(data.exit_code);
                        }
                    }
                })
                .catch(function (error) {
                    console.error('実行ログの取得に失敗しました:', error);
                });
        }

        function start(newJobId) {
            if (!newJobId) { return; }
            stop();
            jobId = newJobId;
            offset = 0;
            seenKeys = {};
            issues = [];
            finished = false;
            if (logElement) { logElement.textContent = ''; }
            if (issueElement) { issueElement.innerHTML = ''; }
            poll();
            timer = setInterval(poll, POLL_INTERVAL_MS);
        }

        function stop() {
            if (timer) {
                clearInterval(timer);
                timer = null;
            }
        }

        // 完了直後の数行を取りこぼさないよう、停止前に最後の1回だけ読む。
        // 既に終了を検知していれば、読むものは残っていないので何もしない。
        function flush() {
            if (!finished) {
                poll();
            }
        }

        return { start: start, stop: stop, flush: flush };
    }

    return {
        createViewer: createViewer,
        renderIssues: renderIssues
    };
}());
