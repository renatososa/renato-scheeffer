// js/efdi.js
(() => {
    const area = document.getElementById('content-area');
    if (!area) return;
  
    // Carpeta donde están los fragmentos HTML (ajustá si es necesario)
    const BASE = 'efdi/';
  
    function setActive(h) {
      document.querySelectorAll('.toc-link')
        .forEach(a => a.classList.toggle('active', a.getAttribute('href') === '#' + h));
    }
  
    async function loadFromHash() {
      const h = location.hash.slice(1) || 'MT01'; // default
      setActive(h);
  
      const path = `${BASE}${h}.html`;
      try {
        const url = new URL(path, window.location); // relativo a efdi.html
        const res = await fetch(url, { cache: 'no-store' });
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        area.innerHTML = await res.text();
        // desplazamiento suave al inicio del contenido cargado
        area.scrollIntoView({ behavior: 'smooth', block: 'start' });
      } catch (err) {
        area.innerHTML = `
          <div class="alert alert-danger">
            No se pudo cargar <code>${path}</code> (${String(err)}).
          </div>`;
      }
    }
  
    window.addEventListener('hashchange', loadFromHash);
    window.addEventListener('DOMContentLoaded', loadFromHash);
  })();
  