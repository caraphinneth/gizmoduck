(function()
{
    css = document.createElement('style');
    css.type = 'text/css';
    css.id = 'Custom CSS';
    document.head.appendChild(css);
    css.innerText = '*:focus {outline: none !important;} a {color: #17bf63; text-decoration-skip-ink: none !important;}';
})();
