document.addEventListener("DOMContentLoaded", () => {
    // Cargar navbar y footer
    fetch("nav.html")
    .then(res => res.text())
    .then(data => {
        document.getElementById("navbar-container").innerHTML = data;

        // activar buscador después de cargar el nav
        const searchForm = document.getElementById("searchForm");
        const searchInput = document.getElementById("searchInput");

        if (searchForm) {
        searchForm.addEventListener("submit", function (e) {
            e.preventDefault();
            const query = searchInput.value.trim().toLowerCase();
            if (!query) return;

            // Buscar coincidencia en el texto visible de la página actual
            const elements = document.querySelectorAll("h1, h2, h3, h4, p, a, li, span, div, section");
            let found = false;

            for (let el of elements) {
            if (el.textContent.toLowerCase().includes(query)) {
                el.scrollIntoView({ behavior: "smooth", block: "center" });
                el.style.backgroundColor = "yellow"; // resaltar
                setTimeout(() => el.style.backgroundColor = "", 2000);
                found = true;
                break;
            }
            }

            if (!found) {
            alert("No se encontraron resultados para: " + query);
            }
        });
        }
    });

    fetch("footer.html")
    .then(res => res.text())
    .then(data => document.getElementById("footer-container").innerHTML = data);

    // Formulario contacto
    document.addEventListener("submit", function(e){
    if (e.target.id === "contactForm") {
        e.preventDefault();
        alert("¡Gracias por tu mensaje! Te responderé pronto.");
        e.target.reset();
    }
    });
});  
document.addEventListener('click', (e) => {
    const a = e.target.closest('a.toc-link');
    if (!a) return;
    const target = document.querySelector(a.getAttribute('href'));
    if (target) {
        e.preventDefault();
        target.scrollIntoView({ behavior: 'smooth', block: 'start' });
        // En móviles, al hacer clic cerramos el acordeón abierto para ver el contenido
        const coll = a.closest('.accordion-collapse');
        if (window.innerWidth < 992 && coll) {
        const bsColl = bootstrap.Collapse.getOrCreateInstance(coll);
        setTimeout(() => bsColl.hide(), 200);
        }
    }
    });