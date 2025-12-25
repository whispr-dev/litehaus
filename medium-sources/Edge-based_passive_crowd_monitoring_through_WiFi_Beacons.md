Elsevier
Computer Communications
Volume 192, 1 August 2022, Pages 163-170
Computer Communications
Edge-based passive crowd monitoring through WiFi Beacons
Author links open overlay panel
Kalkidan Gebru
, 
Marco Rapelli
, 
Riccardo Rusca
, Claudio Casetti
, Carla Fabiana Chiasserini
, Paolo Giaccone

Show more

Add to Mendeley

Share

Cite
https://doi.org/10.1016/j.comcom.2022.06.003
Get rights and content
Abstract
Tracking people’s flows has become crucial, not only for safety and security, but also for numerous practical business applications and better management of urban spaces, facilities and services. In this paper, we proposed methodologies that, exploiting IoT technology deployed at the edge of the network, allow for the analysis of people’s movement in urban environments, both outdoors and indoors. In particular, leveraging the use of WiFi probe packets sent by smart devices carried by people on the move, we first describe an implementation of our methodology using off-the-shelf hardware to count people boarding public transportation vehicles. We then present an alternate implementation using commercial WiFi scanners connected to the edge and leveraging suitably deployed virtual network functions to process the data collected by a OneM2M IoT platform, proposing also a mobility tracking procedure that can be applied to anonymized data provided by commercial WiFi scanners. Our experimental results show that the proposed approaches to people counting and mobility detection can achieve a good level of accuracy, while overall carrying a low price tag.
Introduction
In November 2018, the Italian National Institute of Statistics (ISTAT) published a research on everyday commuters and their means of transportation [1]. Data from the prior year’s national census was used in the analysis. According to the findings of this investigation, more than 40% of the Italian population prefers not to use private transportation for daily travels on a regular basis, 19% of the Italian population prefers to go by foot or bicycle, while 23% prefers to use public transportation such as buses, trams, trains, school buses, or subway. Overall, this represents a considerable proportion of national commuters who rely on non-private form of transportation. To go to and from work or school on a regular basis, this section of the urban population relies on adequate infrastructure and services provided by municipal transportation agencies. It is clear that basic data acquired from a census will not be sufficient to achieve this aim. In order to maintain a high level of service, precise monitoring of the number of users of public transportation is required on a regular basis. If the local transportation authority had real-time data on bus passengers collected at each stop, they would be able to expand the service appropriately. In addition, following the global lockdown imposed by governments to stop the spread of the COVID-19 pandemic, capacity restrictions in public places were established to limit the number of individuals who gathered. As an example, in Italy a capacity limit for the maximum number of people that can be transported on public transit vehicles was established. It is therefore clear that designing an efficient system for people’s flow monitoring becomes critical. However, fulfilling this need in a trustworthy manner is highly challenging, as the privacy of the data of every individual involved should be preserved in every part of the designed monitoring system.
Importantly, for many safety applications and convenience services designed for mobile users it is essential to detect the pattern taken by people’s flows at different times of the day/week. One of the key technologies to achieve this goal is the Internet-of-Things (IoT) [2], [3], as IoT devices are becoming pervasive and most of them are equipped with a radio interface, e.g., WiFi, LoRaWan or 5G, that can conveniently connect them with other devices as well as with the communication network infrastructure. Furthermore, IoT devices typically consume little energy, hence they contribute to creating sustainable communication systems, have low cost, and pose fewer privacy issues than other devices like smart city cameras.
In this work, we leverage the IoT technology and tackle the problem of characterizing both people’s trajectories and the number of people a flow includes, while preserving users’ privacy. In particular, we focus on an urban environment and exploit both commercial sensors and simple devices like Raspberry PIs, equipped with a WiFi interface. Such devices can scan the WiFi spectrum for probe requests, i.e., packets transmitted by user hand-handled devices towards nearby access points. Using the logs provided by these spectrum scanners, we develop techniques to increase the privacy level in data collection and processing. Importantly, we aim at developing a solution that can cope with the serious limitations of commercial or Raspberry PI-based scanners, which demands for a new approach with respect to those proposed in prior art.
Unlike existing work, we develop mechanisms that can effectively (i) cope with commercial sensors as well as simple, low-cost ad-hoc designed devices that scan the spectrum for WiFi probes, and (ii) increase the level of users’ privacy protection. Further, applying an ML-based scheme, we show how the data collected through our privacy-preserving technique can be used to characterize people’s flows. Our approach is then validated through a proof-of-concept testbed that we developed and a measurement campaign that we performed on public buses run by GTT [4] in the city of Turin, Italy.
The rest of the paper is organized as follows. After discussing some relevant related work in Section 2, in Section 3 we describe some features of the WiFi technology that are relevant for the design of our privacy-preserving solution for people’s flows detection. Then Section 4 proposes an approach for people’s counting indoors, taking a public bus as an example of indoor public places where a monitoring system could be deployed. The methodology we use for outdoor spaces is instead presented in Section 5. An approach for people’s mobility tracking is introduced in Section 6. Finally, Section 7 draws some conclusions.
Section snippets
Related work
In recent years, several different approaches have been used to face the problem of detecting and counting people in an urban area, both in indoor and outdoor scenario. Infrared sensors, cameras, pressure sensors, visible light sensors, RFID, UWB, and audio-processing are some of them, however, the techniques mentioned above do not provide satisfactory results at times in relation to the cost of implementation while at other times they obtain insufficient performance [5].
Some approaches are
Preliminaries and main observations
As mentioned, we use WiFi signal reception and processing to estimate the number of smart devices (e.g., smartphone, tablet, laptop, smartwatch, etc.). Our approach thus consists in scanning the WiFi spectrum, attempting to capture WiFi packets sent by smartphones. When a smartphone’s WiFi interface is turned on, it transmits a burst of broadcast messages to discover Access Points (APs) and smartphones nearby. To link a recorded WiFi signal with a smartphone, we examine the MAC (Media Access
Indoors crowd monitoring: People counting on a bus
In this section, we focus on counting people traveling on a public bus in the city of Turin. Thanks to a very low cost Raspberry Pi 3B, we apply a de-randomization algorithm to count in real-time the number of passengers on-board, using the probe request broadcasted over the WiFi network by the passengers’ devices.
Outdoors crowd monitoring
We now tackle an outdoor scenario, focusing on detecting and counting people moving by any means of transportation in a specific area of Turin, Italy. In this testbed, several of commercial scanners were installed along a mile between the Politecnico di Torino campus and the Porta Susa train station, which is the major transit train station of the city of Turin, Italy. In this area, we were able to detect people who commute everyday to our campus on foot, but also a large inflow and outflow of
Tracking mobility
In this section, we introduce a methodology for detecting and also tracking smart devices, that can be applied in the testbed scenario described in Section 5. This methodology is apt to be applied to counting procedures using black-box scanners, as those in the previous section, but it is also amenable to do-it-yourself solutions as the ones described in Section 4.
Conclusions
Detecting the presence of people in outdoors and indoors areas is useful in several scenarios: from the design of urban spaces, to the planning of public transportation, to the implementation of mobility restriction measures such as the one that many countries put in place during the COVID-19 pandemic. In this paper, we have presented two possible methodologies for people counting and mobility detection based both on off-the-shelf hardware and commercial devices. We have shown that, although
Declaration of Competing Interest
The authors declare the following financial interests/personal relationships which may be considered as potential competing interests: We would like to remark that there are conflicts of interest among the authors of our submission “Edge-based Passive Crowd Monitoring Through WiFi Beacons” and the Associate Editor-in-Chief of this journal.
Acknowledgment
This work was supported by the EU Commission through the RAINBOW project (Grant 871403).
References (22)
LongoE. et al.
Accurate occupancy estimation with WiFi and bluetooth/BLE packet capture
Comput. Netw.
(2019)
Spostamenti Quotidiani e Nuove Forme di MobilitàStatistical document
(2018)
BadiiC. et al.
Sii-mobility: An IoT/IoE architecture to enhance smart city mobility and transportation services
MDPI Sensors
(2019)
UrasM. et al.
PmA: a solution for people mobility monitoring and analysis based on WiFi probes
GTT - Gruppo Torinese Trasporti,...
OliveiraL. et al.
Mobile device detection through WiFi probe request analysis
IEEE Access
(2019)
WangF. et al.
Passive people counting using commodity WiFi
YangY. et al.
Wi-Count: Passing people counting with COTS WiFi devices
YOLOv3,...
GebruK. et al.
IoT-based mobility tracking for smart city applications

View more references
Cited by (17)
CrowdWatch: Privacy-Preserving Monitoring Leveraging WiFi Multiple Access Information
2025, IEEE Internet of Things Journal
Analyzing Broadcast Patterns and Randomization Techniques in Wi-Fi Probe Request Frames
2025, IEEE Network
Using Wi-Fi Probes to Evaluate the Spatio-Temporal Dynamics of Tourist Preferences in Historic Districts’ Public Spaces
2024, ISPRS International Journal of Geo Information
Privacy-preserving WiFi-based crowd monitoring
2024, Transactions on Emerging Telecommunications Technologies
A Tutorial on Privacy, RCM and Its Implications in WLAN
2024, IEEE Communications Surveys and Tutorials
Non-Intrusive Privacy-Preserving Approach for Presence Monitoring Based on WiFi Probe Requests
2023, Sensors
View all citing articles on Scopus
View full text
© 2022 Elsevier B.V. All rights reserved.