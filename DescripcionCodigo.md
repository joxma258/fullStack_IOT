como primera fase de este lado del proyecto saque una base de datos de consumo electrico en específico me base en el dataset
Individual Household Electric Power Consumption
Donated on 8/29/2012
Hebrail, G. & Berard, A. (2006). Individual Household Electric Power Consumption [Dataset]. UCI Machine Learning Repository. https://doi.org/10.24432/C58K54.


a partir de ello hice un promedio de consumo energetico de cada dia de la semana utilizando PTHON, organizado por horas y dias de la semana empezado por 0 que representa el dia lunea
y 6 que representa el dia domingo, 
de forma paralela 0 en horas representa 00:00 y 24 representa 24:00, utilizo este nuevo data set en esp32 para poder basarme en el consumo energético que envíare por MQTT

el lagoritmo como primer paso se conecta a la internet, luego hace una consulta de que hora es tomando en cuenta el dia y la hora de esta consulta, el algoritmo busca los valores de 
energía que estan registrados en el dataset, y añade cierta cantidad de ruido aleatorea que se le suma al valor de la base de datos y lo imprime en el SERIAL 

finalmente este mensaje que envia desde el serial tambien lo envía mediante MQTT
 NOTA: realmente MQTT tiene una version mejorada en seguridad el MQTTS que encripta los mensajes y se asegura que se reciban correctamente al destinatario, si se tratara de un proyecto en donde 
 la esp32 se conectaría a una red externa es NECESARIO utilizar alguna clase de seguridad o implementar el servicio MQTT sobre WebSocket, en este ejemplo yo NO lo hago
