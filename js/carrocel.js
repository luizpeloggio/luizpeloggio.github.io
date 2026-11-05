$(document).ready(function () {
    const $track =$(".tech-track");
    let currentX = 0;
    const speed = 1.2; // Velocidade do movimento (quanto maior, mais rápido)

    // Função de animação contínua
    function step() {
        currentX -= speed;
        
        // Quando o primeiro bloco inteiro passar pela tela, reseta a posição instantaneamente
        const halfWidth = $track[0].scrollWidth / 2;
        if (Math.abs(currentX) >= halfWidth) {
            currentX = 0;
        }

        $track.css("transform", `translateX(${currentX}px)`);
        requestAnimationFrame(step);
    }

    // Inicia a animação do carrossel
    let animationId = requestAnimationFrame(step);

    // Opcional: Pausa quando o mouse passa por cima
    $(".tech-carousel-container").hover(
        function () {
            cancelAnimationFrame(animationId);
        },
        function () {
            animationId = requestAnimationFrame(step);
        }
    );
});