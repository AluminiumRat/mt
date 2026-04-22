//  Буфер для временного хранения промежуточных результатов фильтрации
layout (r11f_g11f_b10f,
        set = VOLATILE,
        binding = 0) uniform image2D sourceImage;

layout (r11f_g11f_b10f,
        set = VOLATILE,
        binding = 1) uniform image2D targetImage;