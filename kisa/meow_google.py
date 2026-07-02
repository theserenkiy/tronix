import numpy as np
import scipy.io.wavfile as wav

# 1. Основные параметры звука
sample_rate = 44100  # Частота дискретизации (Гц)
duration = 0.8       # Длительность звука «мяу» (секунд)
t = np.linspace(0, duration, int(sample_rate * duration), endpoint=False)

# 2. Моделирование частотной огибающей (Управление частотой генератора)
# В железе это будет RC-цепочка, задающая напряжение для ГУН (VCO)
# Частота сначала быстро растет, а затем падает
f_start = 350.0  # Начальная частота (Гц)
f_peak = 750.0   # Пиковая частота (Гц)
f_end = 250.0    # Финальная частота (Гц)
t_peak = 0.25    # Время достижения пика (секунд)

# Строим кривую изменения частоты (плавный изгиб)
frequency_profile = np.where(
    t < t_peak,
    f_start + (f_peak - f_start) * (t / t_peak)**1.5,
    f_peak - (f_peak - f_end) * ((t - t_peak) / (duration - t_peak))**1.2
)

# Интегрируем частоту, чтобы получить фазу для генератора
phase = 2 * np.pi * np.cumsum(frequency_profile) / sample_rate

# 3. Генератор исходной волны (VCO)
# Кошачий голос богат гармониками, поэтому берем треугольную волну (Triangle wave).
# На чистой синусоиде «мяу» не получится, а пила будет слишком резкой.
raw_wave = 2 * np.abs(2 * (phase / (2 * np.pi) - np.floor(phase / (2 * np.pi) + 0.5))) - 1

# 4. Моделирование огибающей громкости (VCA / Генератор ADSR)
# Быстрая атака (Attack) и плавный затухающий спад (Decay/Release)
attack_time = 0.1
amplitude_envelope = np.where(
    t < attack_time,
    (t / attack_time),
    np.exp(-4 * (t - attack_time) / (duration - attack_time))
)

# 5. Динамический фильтр (Имитация форманты «Мя-у»)
# В аналоге это фильтр низких частот (Low-Pass Filter), частота среза которого 
# движется синхронно с громкостью. Приглушает резкие ВЧ в начале и конце.
cutoff_profile = 400 + 1800 * amplitude_envelope
filtered_wave = np.zeros_like(raw_wave)

# Простейшая цифровая симуляция RC-фильтра первого порядка
alpha = 2 * np.pi * cutoff_profile / sample_rate
alpha = np.clip(alpha, 0, 1)
for i in range(1, len(raw_wave)):
    filtered_wave[i] = filtered_wave[i-1] + alpha[i] * (raw_wave[i] - filtered_wave[i-1])

# 6. Финальный микс и нормализация
meow_sound = filtered_wave * amplitude_envelope
meow_sound = meow_sound / np.max(np.abs(meow_sound))  # Нормализация под 100% громкости
audio_data = (meow_sound * 32767).astype(np.int16)     # Конвертация в 16-bit PCM

# Сохранение файла для прослушивания
wav.write("analog_meow_model.wav", sample_rate, audio_data)
print("Файл analog_meow_model.wav успешно сгенерирован!")