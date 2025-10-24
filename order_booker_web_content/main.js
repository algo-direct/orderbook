
document.addEventListener('DOMContentLoaded', function () {
    console.log('DOM fully loaded and parsed.');

    const obBids = document.getElementsByClassName('ob-side-bids')[0];
    const obAsks = document.getElementsByClassName('ob-side-asks')[0];
    const dateTimeHeading = document.getElementsByClassName('date-time')[0];

    function populateGridUsingSnapshot(snapshot) {
        const populateLevels = (title, div, levels) => {
            div.innerHTML = '';
            const levelsHeader = document.createElement('div');
            levelsHeader.classList.add('ob-header');
            levelsHeader.innerHTML = title;
            div.appendChild(levelsHeader);
            levels.forEach(level => {
                const levelDiv = document.createElement('div');
                levelDiv.classList.add('ob-level');
                div.appendChild(levelDiv);
                const priceDiv = document.createElement('div');

                priceDiv.innerHTML = `${level[0]}`;
                levelDiv.appendChild(priceDiv);

                const sizeDiv = document.createElement('div');
                let qtyFraction = Number(level[1]);
                if (qtyFraction >= 1) {
                    qtyFraction = 100;
                }
                else {
                    qtyFraction = Math.floor(100 * qtyFraction);
                }
                sizeDiv.textContent = `${level[1]}`;
                const gradientStart = title == "Bids" ? "green" : "red";
                const backgroundImage = `linear-gradient(to right, ${gradientStart} ${qtyFraction}%, black);`;
                sizeDiv.setAttribute('style', `background-image: ${backgroundImage}`);
                levelDiv.appendChild(sizeDiv);
            });
        };
        populateLevels('Bids', obBids, snapshot.bids);
        populateLevels('Asks', obAsks, snapshot.asks);
        const dateTime = (new Date(parseInt(parseInt(snapshot.time))))
            .toLocaleString('en-US', {
                year: 'numeric',
                month: '2-digit',
                day: '2-digit',
                hour: '2-digit',
                minute: '2-digit',
                second: '2-digit',
                fractionalSecondDigits: 3
            });
        dateTimeHeading.innerHTML = dateTime;
    }


    function fetchData() {
        fetch(window.location.pathname + '/snapshot.api')
            .then(response => response.json())
            .then(data => {
                console.log('Data received:', data);
                populateGridUsingSnapshot(data);
            })
            .catch(error => {
                console.error('Error fetching data:', error);
            });
    }

    fetchData();
    const intervalId = setInterval(fetchData, 500);
});