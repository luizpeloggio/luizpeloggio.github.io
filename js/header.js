$(document).ready(function () {
    function initMenu() {
        let $links = $(".menu .menu-single ul li a");

        // Remove de todos e aplica no primeiro (índice 0)
        $links.removeClass("ativeMenu");
        $links.eq(0).addClass("ativeMenu");

        // Evento de clique unificado
        $links.off("click").on("click", function() {

            $links.removeClass("ativeMenu");
            $(this).addClass("ativeMenu");
        });
    } 

    initMenu();

    function initScroll() {
        $('.menu .menu-singleul li a[href^="#"]').on("click", function (e) {
        e.preventDefault();

        const destino = $($(this).attr("href"))[0];
        destino.scrollIntoView({ behavior: "smooth" });
        });
    }initScroll();
    
});