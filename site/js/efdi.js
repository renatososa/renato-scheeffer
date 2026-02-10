// js/efdi.js
// Genera IDs únicos tipo slug si el heading no tiene id
function slugify(text) {
  return text.toString().toLowerCase()
    .trim()
    .replace(/[áàäâ]/g, 'a').replace(/[éèëê]/g, 'e')
    .replace(/[íìïî]/g, 'i').replace(/[óòöô]/g, 'o')
    .replace(/[úùüû]/g, 'u').replace(/ñ/g, 'n')
    .replace(/[^a-z0-9\s-]/g, '')
    .replace(/\s+/g, '-')
    .replace(/-+/g, '-');
}

function buildTOC() {
  const root = document.getElementById('content-area');
  const tocList = document.getElementById('toc-list');
  if (!root || !tocList) return;

  // Limpiar TOC previo
  tocList.innerHTML = '';

  // Tomar todos los H1..H6 en el contenido dinámico
  const headings = root.querySelectorAll('h1, h2, h3, h4, h5, h6');
  if (!headings.length) {
    tocList.innerHTML = '<span class="text-muted">No hay encabezados</span>';
    return;
  }

  // Asegurar que cada heading tenga ID
  const used = new Set();
  headings.forEach(h => {
    if (!h.id) {
      let base = slugify(h.textContent || 'section');
      let id = base;
      let i = 2;
      while (used.has(id) || document.getElementById(id)) {
        id = `${base}-${i++}`;
      }
      h.id = id;
      used.add(id);
    }
  });

  // Construir TOC jerarquizado
  let lastLevel = 1; // Para indentación (H1 = nivel 1)
  headings.forEach(h => {
    const level = parseInt(h.tagName.substring(1), 10);
    const a = document.createElement('a');
    a.href = `#${h.id}`;
    a.className = `toc-link nav-link toc-indent-${Math.max(0, level - 1)}`;
    a.textContent = h.textContent.trim();

    // Scroll suave
    a.addEventListener('click', (e) => {
      e.preventDefault();
      const target = document.getElementById(h.id);
      if (target) {
        history.pushState(null, '', `#${h.id}`);
        const y = target.getBoundingClientRect().top + window.pageYOffset - 80; // offset por navbar
        window.scrollTo({ top: y, behavior: 'smooth' });
      }
    });

    tocList.appendChild(a);
    lastLevel = level;
  });

  // Resaltar activo con IntersectionObserver
  observeActiveHeadings(headings);
}

let headingObserver;
function observeActiveHeadings(headings) {
  // Desconectar observer previo
  if (headingObserver) headingObserver.disconnect();

  const links = Array.from(document.querySelectorAll('#toc-list .toc-link'));
  const map = new Map(); // id -> link
  links.forEach(l => map.set(l.getAttribute('href').slice(1), l));

  headingObserver = new IntersectionObserver((entries) => {
    // El que tenga mayor intersección visible será el activo
    let best = null;
    entries.forEach(entry => {
      if (entry.isIntersecting) {
        if (!best || entry.intersectionRatio > best.intersectionRatio) {
          best = entry;
        }
      }
    });
    if (best) {
      const id = best.target.id;
      links.forEach(l => l.classList.remove('active'));
      const active = map.get(id);
      if (active) active.classList.add('active');
    }
  }, {
    rootMargin: '-100px 0px -60% 0px', // activa un poco antes
    threshold: [0, 0.25, 0.5, 0.75, 1]
  });

  headings.forEach(h => headingObserver.observe(h));
}

(() => {
    const area = document.getElementById('content-area');
    if (!area) return;
  
    // Carpeta donde están los fragmentos HTML (ajustá si es necesario)
    const BASE = 'efdi/';
  
    function setActive(h) {
      document.querySelectorAll('.toc-link')
        .forEach(a => a.classList.toggle('active', a.getAttribute('href') === '#' + h));
    }
    function syncAccordion(h) {
      // Inferir prefijo (MT/MD) y componer IDs
      const pref = h.slice(0, 2); // 'MT', 'MD' o 'MI'
      const accId = '#acc'+pref;
      const targetCollapse = '#c' + h; // ej: #cMT01

      // Cerrar todos en ambos accordions
      document.querySelectorAll('.accordion-collapse.show')
        .forEach(el => bootstrap.Collapse.getOrCreateInstance(el).hide());

      // Abrir el que corresponde si existe
      const el = document.querySelector(targetCollapse);
      if (el) bootstrap.Collapse.getOrCreateInstance(el).show();

      // Marcar botón activo visualmente
      document.querySelectorAll('.accordion-button').forEach(btn => {
        btn.classList.toggle('active', btn.getAttribute('data-bs-target') === targetCollapse);
      });
    }
  
    async function loadFromHash() {
      const h = location.hash.slice(1) || 'MT01'; // default
      setActive(h);
      syncAccordion(h);
      const path = `${BASE}${h}.html`;
      try {
        const url = new URL(path, window.location); // relativo a efdi.html
        const res = await fetch(url, { cache: 'no-store' });
        if (!res.ok) throw new Error(`HTTP ${res.status}`);
        area.innerHTML = await res.text();
        buildTOC();
        // desplazamiento suave al inicio del contenido cargado
        area.scrollIntoView({ behavior: 'smooth', block: 'start' });
      } catch (err) {
        area.innerHTML = `
          <div class="alert alert-danger">
            No se pudo cargar <code>${path}</code> (${String(err)}).
          </div>`;
      }
      // --- sincroniza el accordion con el hash seleccionado ---
    

    }
    window.addEventListener('hashchange', loadFromHash);
    window.addEventListener('DOMContentLoaded', loadFromHash);
  })();
  