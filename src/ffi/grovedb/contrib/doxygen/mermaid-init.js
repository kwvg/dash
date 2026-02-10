/**
 * Mermaid diagram loader with dark-mode re-render support.
 *
 * Diagrams are embedded as <div class="mermaid" data-mermaid-source="BASE64">.
 * This script loads the Mermaid CDN library, renders each diagram, and
 * re-renders when the doxygen-awesome dark-mode toggle fires.
 */
(function () {
    'use strict';

    var CDN = 'https://cdn.jsdelivr.net/npm/mermaid@11/dist/mermaid.min.js';
    var renderId = 0;

    function isDark() {
        return document.documentElement.classList.contains('dark-mode');
    }

    function fragmentBg() {
        return getComputedStyle(document.documentElement)
            .getPropertyValue('--fragment-background').trim() || '#f5f5f5';
    }

    /* GroveDB palette — must use 'base' theme for themeVariables to apply */
    var LIGHT_VARS = {
        primaryColor: '#e6f3fb',
        primaryTextColor: '#2c3e50',
        primaryBorderColor: '#0289DD',
        secondaryColor: '#b8dff5',
        secondaryTextColor: '#2c3e50',
        secondaryBorderColor: '#0270B8',
        tertiaryColor: '#f5f5f5',
        tertiaryTextColor: '#67727e',
        tertiaryBorderColor: '#e0e0e0',
        lineColor: '#0289DD',
        textColor: '#2c3e50',
        mainBkg: '#e6f3fb',
        nodeBorder: '#0289DD',
        nodeTextColor: '#2c3e50',
        clusterBkg: '#f5f5f5',
        clusterBorder: '#b8dff5',
        edgeLabelBackground: '#ffffff',
        noteBkgColor: '#e6f3fb',
        noteTextColor: '#2c3e50',
        noteBorderColor: '#0289DD',
        actorBkg: '#e6f3fb',
        actorBorder: '#0289DD',
        actorTextColor: '#2c3e50'
    };

    var DARK_VARS = {
        darkMode: true,
        primaryColor: '#182E3C',
        primaryTextColor: '#e0e0e0',
        primaryBorderColor: '#70bdec',
        secondaryColor: '#1a3d55',
        secondaryTextColor: '#e0e0e0',
        secondaryBorderColor: '#2a7ab5',
        tertiaryColor: '#1c1c1c',
        tertiaryTextColor: '#a0a8b0',
        tertiaryBorderColor: '#2a2a2a',
        lineColor: '#70bdec',
        textColor: '#e0e0e0',
        mainBkg: '#182E3C',
        nodeBorder: '#70bdec',
        nodeTextColor: '#e0e0e0',
        clusterBkg: '#1c1c1c',
        clusterBorder: '#2a7ab5',
        edgeLabelBackground: '#141414',
        noteBkgColor: '#182E3C',
        noteTextColor: '#e0e0e0',
        noteBorderColor: '#70bdec',
        actorBkg: '#182E3C',
        actorBorder: '#70bdec',
        actorTextColor: '#e0e0e0'
    };

    function decodeBase64UTF8(b64) {
        var bytes = Uint8Array.from(atob(b64), function (c) { return c.charCodeAt(0); });
        return new TextDecoder().decode(bytes);
    }

    function renderAll() {
        var divs = document.querySelectorAll('div.mermaid[data-mermaid-source]');
        if (!divs.length) return;

        var vars = isDark() ? Object.assign({}, DARK_VARS) : Object.assign({}, LIGHT_VARS);
        vars.edgeLabelBackground = fragmentBg();

        mermaid.initialize({
            startOnLoad: false,
            theme: 'base',
            themeVariables: vars
        });

        divs.forEach(function (div) {
            var source = decodeBase64UTF8(div.getAttribute('data-mermaid-source'));
            var id = 'mermaid-diagram-' + (++renderId);
            mermaid.render(id, source).then(function (result) {
                div.innerHTML = result.svg;
            }).catch(function (err) {
                div.innerHTML = '<pre class="mermaid-error">' +
                    err.toString().replace(/</g, '&lt;') + '</pre>';
            });
        });
    }

    function loadScript(url, cb) {
        var s = document.createElement('script');
        s.src = url;
        s.onload = cb;
        document.head.appendChild(s);
    }

    document.addEventListener('DOMContentLoaded', function () {
        if (!document.querySelector('div.mermaid[data-mermaid-source]')) return;

        loadScript(CDN, function () {
            renderAll();

            // Re-render on dark-mode toggle (class change on <html>)
            new MutationObserver(function (mutations) {
                mutations.forEach(function (m) {
                    if (m.attributeName === 'class') renderAll();
                });
            }).observe(document.documentElement, { attributes: true });
        });
    });
})();
