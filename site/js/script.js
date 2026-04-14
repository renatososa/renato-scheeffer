document.addEventListener("DOMContentLoaded", () => {
    fetch("nav.html", { cache: "no-store" })
        .then((res) => res.text())
        .then((data) => {
            document.getElementById("navbar-container").innerHTML = data;

            const searchForm = document.getElementById("searchForm");
            const searchInput = document.getElementById("searchInput");
            const searchPanel = document.getElementById("searchPanel");
            const searchToggle = document.getElementById("searchToggle");
            const searchToggleIcon = document.getElementById("searchToggleIcon");

            function setSearchOpen(isOpen) {
                if (!searchPanel || !searchToggle) return;

                searchPanel.hidden = !isOpen;
                searchPanel.classList.toggle("is-open", isOpen);
                searchToggle.setAttribute("aria-expanded", String(isOpen));
                searchToggle.setAttribute("aria-label", isOpen ? "Cerrar buscador" : "Abrir buscador");
                document.body.classList.toggle("search-open", isOpen);

                if (searchToggleIcon) {
                    searchToggleIcon.className = isOpen ? "bi bi-x-lg" : "bi bi-search";
                }

                if (isOpen && searchInput) {
                    requestAnimationFrame(() => searchInput.focus());
                }
            }

            if (searchToggle) {
                searchToggle.addEventListener("click", () => {
                    const isOpen = searchPanel && !searchPanel.hidden;
                    setSearchOpen(!isOpen);
                });
            }

            document.addEventListener("keydown", (e) => {
                if (e.key === "Escape") {
                    setSearchOpen(false);
                }
            });

            document.addEventListener("click", (e) => {
                const clickInsidePanel = searchPanel && searchPanel.contains(e.target);
                const clickOnToggle = searchToggle && searchToggle.contains(e.target);

                if (!clickInsidePanel && !clickOnToggle) {
                    setSearchOpen(false);
                }
            });

            if (searchForm && searchInput) {
                searchForm.addEventListener("submit", (e) => {
                    e.preventDefault();
                    const query = searchInput.value.trim().toLowerCase();
                    if (!query) return;

                    const elements = document.querySelectorAll("h1, h2, h3, h4, p, a, li, span, div, section");
                    let found = false;

                    for (const el of elements) {
                        if (el.textContent.toLowerCase().includes(query)) {
                            el.scrollIntoView({ behavior: "smooth", block: "center" });
                            el.style.backgroundColor = "yellow";
                            setTimeout(() => {
                                el.style.backgroundColor = "";
                            }, 2000);
                            found = true;
                            setSearchOpen(false);
                            break;
                        }
                    }

                    if (!found) {
                        alert("No se encontraron resultados para: " + query);
                    }
                });
            }
        });

    fetch("footer.html", { cache: "no-store" })
        .then((res) => res.text())
        .then((data) => {
            document.getElementById("footer-container").innerHTML = data;
        });

    document.addEventListener("submit", (e) => {
        if (e.target.id === "contactForm") {
            e.preventDefault();
            alert("¡Gracias por tu mensaje! Te responderé pronto.");
            e.target.reset();
        }
    });
});

document.addEventListener("click", (e) => {
    const a = e.target.closest("a.toc-link");
    if (!a) return;

    const target = document.querySelector(a.getAttribute("href"));
    if (target) {
        e.preventDefault();
        target.scrollIntoView({ behavior: "smooth", block: "start" });

        const coll = a.closest(".accordion-collapse");
        if (window.innerWidth < 992 && coll) {
            const bsColl = bootstrap.Collapse.getOrCreateInstance(coll);
            setTimeout(() => bsColl.hide(), 200);
        }
    }
});
