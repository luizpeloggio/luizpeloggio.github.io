$(document).ready(function() {

    function clickOpen(){
        let $curiosity = $(".curiosity .container .curiosity-painel");
        $curiosity.off('click').on('click',function(){

            console.log("oi");
            $(this).toggleClass("activePainel");
            $curiosity.not(this).removeClass("activePainel");
        });
    }clickOpen();
    
    function clickFooterLink(){

        $(".footer-contact .footer-body .footer-single img").on("click", function () {
        let url = $(this).attr("link"); // Pega o link guardado no atributo
        if (url) {
            window.open(url, "_blank"); // Abre em uma nova aba
        }
    });

    }clickFooterLink();


});

