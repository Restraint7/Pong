#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <SFML/System.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Main.hpp>
#include <random>
#include <iostream>
#include <string>
#include <cmath>
#include <list>

const float LENGTHOFTHEWINDOW = 800.f;
const float HEIGHTHTOFTHEWINDOW = LENGTHOFTHEWINDOW * 0.75;
const float SPEEDOFPADDLEPERSECOND = LENGTHOFTHEWINDOW/3.2;
const float SPEEDOFBALLPERSECOND= LENGTHOFTHEWINDOW/2;
const float SPEEDOFSETTINGBLOCK = 1;
const float LENGTHOFTHEPADDLE = LENGTHOFTHEWINDOW / 8;
const float WIDTHOFTHEPADDLE = LENGTHOFTHEPADDLE/ 5;
const float WIDTHOFTHEINTERVAL = WIDTHOFTHEPADDLE / 2;
const float RADIUSOFTHEBALL = WIDTHOFTHEPADDLE / 2;
const float LENGTHOFTHESETTINGBLOCK = LENGTHOFTHEWINDOW * 0.04;
const float HEIGHTHOFTHESETTINGBLOCK = HEIGHTHTOFTHEWINDOW *0.04;
const float LEFTCOLLISIONLINE = WIDTHOFTHEINTERVAL + WIDTHOFTHEPADDLE;
const float RIGHTCOLLISIONLINE = LENGTHOFTHEWINDOW - LEFTCOLLISIONLINE;
const float LENGTHOFAIGIVEUPESTIMATE = LENGTHOFTHEWINDOW / 8;
const int FONTSIZEOFSCOREBOARD = LENGTHOFTHEWINDOW / 16;
const int FONTSIZEOFSETTING = LENGTHOFTHEWINDOW / 50;
const int WINSCORE = 5;
enum class AlignOfText{leftAlign,center,rightAlign};
enum class BorderShape{circle,diamond,noBorder};
sf::Sound soundOfHit;
sf::SoundBuffer buffer;


class Ball:public sf::CircleShape {
public:
	sf::Vector2f normalizationVelocity;
	sf::Vector2f velocity;
	void setARandomStartSpeed() {
		/*velocity.x = -0.016;
		velocity.y = -0.0128;*/
		//get a random speed of the ball, Vx^2+Vy^2 = V^2,Vy<=0.8V,to avoid Vx <=0.6V and the game will be boring.
		std::default_random_engine generator(time(NULL));
		std::uniform_real_distribution<float> distribution(-0.8, 0.8);
		normalizationVelocity.y = distribution(generator);
		normalizationVelocity.x = sqrt(1 - (normalizationVelocity.y) * (normalizationVelocity.y));
		if (generator() % 2) {
			normalizationVelocity.x = -normalizationVelocity.x;
		}
	}
	Ball(sf::Color colorOfTheball,float radiusOfTheBall) {
		setRadius(radiusOfTheBall);
		setOrigin(radiusOfTheBall, radiusOfTheBall);
		setFillColor(colorOfTheball);
	}
};

float estimateHitPosition(Ball ball) {
	sf::Vector2f estimatePositon = sf::Vector2f(ball.getPosition().x + LENGTHOFTHEWINDOW / 8, ball.getPosition().y + (LENGTHOFTHEWINDOW / 8) / ball.normalizationVelocity.x * ball.normalizationVelocity.y);
	if (estimatePositon.y <0 || estimatePositon.y>HEIGHTHTOFTHEWINDOW) {
		if (estimatePositon.y > -HEIGHTHTOFTHEWINDOW && estimatePositon.y < 2 * HEIGHTHTOFTHEWINDOW) {
			// the ball will bounce off one and only one time in 100 px, AI will not error if there is no bounce occur.
			float speedOfTheBall = sqrt(ball.normalizationVelocity.x * ball.normalizationVelocity.x + ball.normalizationVelocity.y * ball.normalizationVelocity.y);
			float biggestErrorAngle = 10 / (180 * (1 + exp(5 - 50 * speedOfTheBall)));
			//AI will estimate the reflectivity angle and it will exist some error according to the speeed of the ball
			//It's a Logistic Equation, when speed < 0.05(perframe), the error will no more than 0.005(1бу), and increase quickly, error will be 5бу when speed = 0.1, and error will no more than 10бу no matter how fast the speed is.
			std::default_random_engine generator(time(NULL));
			std::uniform_real_distribution<float> distribution(-biggestErrorAngle, biggestErrorAngle);
			float errorAngle = distribution(generator);
			if (generator() % 2) {
				if (estimatePositon.y < 0) {
					float tanOrigin = -estimatePositon.y / (LENGTHOFTHEWINDOW / 8);
					estimatePositon.y = (tanOrigin - tan(errorAngle)) / (1 + tanOrigin * tan(errorAngle));
				}
				if (estimatePositon.y > HEIGHTHTOFTHEWINDOW) {
					float tanOrigin = (estimatePositon.y - HEIGHTHTOFTHEWINDOW) / (LENGTHOFTHEWINDOW / 8);
					estimatePositon.y = (tanOrigin + tan(errorAngle)) / (1 - tanOrigin * tan(errorAngle));
				}
			}
		}
		else {
			//if speedY is too big that will bounce off more than once in 100 px, then keep move will the ball
			estimatePositon.y = ball.getPosition().y;
		}
	}
	return estimatePositon.y;
}

class Paddle :public sf::RectangleShape {
public:
	sf::Texture textureOfPaddle;
	sf::Vector2f speedOfPaddle;
	sf::SoundBuffer buffer;
	float width = WIDTHOFTHEPADDLE;
	float length = LENGTHOFTHEPADDLE;
	int errorLength = 0;
	Paddle(sf::Color colorOfThePaddle,float widthOfThePaddle,float lengthOfThePaddle) {
		width = widthOfThePaddle;
		length = lengthOfThePaddle;
		setFillColor(colorOfThePaddle);
		setSize(sf::Vector2f(width, length));
		speedOfPaddle = sf::Vector2f(0, 0);
		textureOfPaddle.loadFromFile("wood.jpg");
		setTexture(&textureOfPaddle);
		errorLength = rand() % int(length * 0.35);
	}
	void controlPaddleByHuman(bool up, bool down, float speedPerFrame) {
		if (up) {
			getPosition().y <= 0 ? speedOfPaddle = sf::Vector2f(0, 0) : speedOfPaddle = sf::Vector2f(0, -speedPerFrame);
			move(speedOfPaddle);
		}
		if (down) {
			getPosition().y >= HEIGHTHTOFTHEWINDOW - length ? speedOfPaddle = sf::Vector2f(0, 0) : speedOfPaddle = sf::Vector2f(0, speedPerFrame);
			move(speedOfPaddle);
		}
	}
	void controlPaddleByAI(Ball& ball, float* estimateYPosition, float speedPerFrame) {
		if (ball.getPosition().x <= LENGTHOFTHEWINDOW - LENGTHOFAIGIVEUPESTIMATE) {
			*estimateYPosition = estimateHitPosition(ball);
		}
		if (getPosition().y + length / 2 - errorLength > *estimateYPosition) {
			controlPaddleByHuman(true, false, speedPerFrame);
		}
		if (getPosition().y + length / 2 - errorLength < *estimateYPosition) {
			controlPaddleByHuman(false, true, speedPerFrame);
		}
	}
};

class TextForPong :public sf::Text {
public:
	sf::Font font;
	sf::CircleShape borderShapeOfText;
	TextForPong() {};
	TextForPong(const sf::String &contentOfTheText, int fontSizeOfTheText,sf::Color colorOfTheText,sf::Vector2f positionOfTheText,AlignOfText alignMethod,BorderShape boarderShape = BorderShape::noBorder) {
		initateTextForPong(contentOfTheText, fontSizeOfTheText, colorOfTheText, positionOfTheText, alignMethod, boarderShape);
	}
	void initateTextForPong(const sf::String& contentOfTheText, int fontSizeOfTheText, sf::Color colorOfTheText, sf::Vector2f positionOfTheText, AlignOfText alignMethod, BorderShape boarderShape = BorderShape::noBorder) {
		font.loadFromFile("arial.ttf");
		setFont(font);
		setString(contentOfTheText);
		setCharacterSize(fontSizeOfTheText);
		setFillColor(colorOfTheText);
		sf::FloatRect borderOfTheText = getLocalBounds();
		switch (alignMethod) {
		case AlignOfText::leftAlign:setPosition(positionOfTheText); break;
		case AlignOfText::center:setOrigin(sf::Vector2f(borderOfTheText.width / 2, 0)); setPosition(positionOfTheText); break;
		case AlignOfText::rightAlign:setOrigin(sf::Vector2f(borderOfTheText.width, 0)); setPosition(positionOfTheText); break;
		}
		borderShapeOfText = addBorder(boarderShape);
	}
	void updatescore(int scoreOfTheLeft, int scoreOfTheRight) {
		std::string scoreboardContent = std::to_string(scoreOfTheLeft) + ":" + std::to_string(scoreOfTheRight);
		setString(scoreboardContent);
	}
	sf::CircleShape addBorder(BorderShape borderShape) {
		sf::FloatRect borderOfTheText = getGlobalBounds();
		float lengthOfTheBorder = sqrt(borderOfTheText.height * borderOfTheText.height + borderOfTheText.width * borderOfTheText.width) / 2;
		switch (borderShape) {
		case BorderShape::noBorder: {
			sf::CircleShape border(lengthOfTheBorder); 
			return border;
			break;
		}
		case BorderShape::circle: {
			sf::CircleShape border(lengthOfTheBorder);
			border.setOrigin(sf::Vector2f(lengthOfTheBorder,lengthOfTheBorder));
			border.setPosition(sf::Vector2f(this->getPosition().x, this->getPosition().y + lengthOfTheBorder / 2 ));
			border.setFillColor(sf::Color::Black);
			border.setOutlineColor(sf::Color::White);
			border.setOutlineThickness(5);
			return border;
			break;
		}
		case BorderShape::diamond: {
			sf::CircleShape border(lengthOfTheBorder*1.25, 4);
			border.setOrigin(sf::Vector2f(lengthOfTheBorder, lengthOfTheBorder));
			border.setPosition(sf::Vector2f(this->getPosition().x -lengthOfTheBorder*0.125, this->getPosition().y + lengthOfTheBorder*1.25 / 2));
			border.setFillColor(sf::Color::Black);
			border.setOutlineColor(sf::Color::White);
			border.setOutlineThickness(5);
			return border;
			break;
		}
		}
	}
};

class binarySetting {
public:
	int serialNumber;
	bool settingOn;
	sf::RectangleShape settingBlock;
	TextForPong settingContent;
	TextForPong on;
	TextForPong off;
	binarySetting(int sNumber,const sf::String &contentOfTheText): 
		settingContent(contentOfTheText, FONTSIZEOFSETTING, sf::Color::White, sf::Vector2f((0.05+0.5* (1 - sNumber % 2))*LENGTHOFTHEWINDOW, 0.05 * ((sNumber+sNumber%2) / 2) * LENGTHOFTHEWINDOW), AlignOfText::leftAlign) ,
		on(std::string("ON"),16,sf::Color::White, sf::Vector2f(LENGTHOFTHEWINDOW * 0.45*(2 - sNumber % 2)- LENGTHOFTHESETTINGBLOCK*2, 0.05 * ((sNumber + sNumber % 2) / 2) * LENGTHOFTHEWINDOW), AlignOfText::leftAlign),
		off(std::string("OFF"), 16, sf::Color::White, sf::Vector2f(LENGTHOFTHEWINDOW * 0.45*(2 - sNumber % 2) - LENGTHOFTHESETTINGBLOCK, 0.05 * ((sNumber + sNumber % 2) / 2) * LENGTHOFTHEWINDOW), AlignOfText::leftAlign)
		//align setting according to the serialnumber, each row has 2 settings
	{
		serialNumber = sNumber;
		settingOn = 0;
		settingBlock.setSize(sf::Vector2f(LENGTHOFTHESETTINGBLOCK,HEIGHTHOFTHESETTINGBLOCK));
		settingBlock.setFillColor(sf::Color::White);
		settingBlock.setPosition(sf::Vector2f(LENGTHOFTHEWINDOW * 0.45 * (2 - serialNumber % 2) - LENGTHOFTHESETTINGBLOCK, 0.05 * ((sNumber + sNumber % 2) / 2) * LENGTHOFTHEWINDOW));
		on.setFillColor(sf::Color::White);
		off.setFillColor(sf::Color::Black);
	}
	void draw(sf::RenderWindow& window) {
		if (settingBlock.getPosition().x >= LENGTHOFTHEWINDOW * 0.45 * (2 - serialNumber % 2) - LENGTHOFTHESETTINGBLOCK * 2 && settingOn == true) {
			settingBlock.move(-SPEEDOFSETTINGBLOCK, 0);
		}
		if (settingBlock.getPosition().x <= LENGTHOFTHEWINDOW * 0.45 * (2 - serialNumber % 2) - LENGTHOFTHESETTINGBLOCK && settingOn == false) {
			settingBlock.move(SPEEDOFSETTINGBLOCK, 0);
		}
		window.draw(settingContent);
		window.draw(settingBlock);
		window.draw(on);
		window.draw(off);
	}
	void whetherSettingChanged(sf::Vector2i positionOfTheMouse) {
		if (positionOfTheMouse.y >= settingBlock.getPosition().y && positionOfTheMouse.y <= settingBlock.getPosition().y + HEIGHTHOFTHESETTINGBLOCK) {
			if (positionOfTheMouse.x >= on.getPosition().x && positionOfTheMouse.x <= on.getPosition().x + LENGTHOFTHESETTINGBLOCK && settingOn == false) {
				on.setFillColor(sf::Color::Black);
				off.setFillColor(sf::Color::White);
				settingOn = !(settingOn);
			}
			if (positionOfTheMouse.x >= off.getPosition().x && positionOfTheMouse.x <= off.getPosition().x + LENGTHOFTHESETTINGBLOCK && settingOn == true) {
				on.setFillColor(sf::Color::White);
				off.setFillColor(sf::Color::Black);
				settingOn = !(settingOn);
			}
		}
	}
};

void initializeTheGame(Ball &ball,Paddle &paddleOne, Paddle &paddleTwo) {
	paddleOne.setPosition(WIDTHOFTHEINTERVAL, HEIGHTHTOFTHEWINDOW/2-LENGTHOFTHEPADDLE/2);
	paddleTwo.setPosition(LENGTHOFTHEWINDOW - WIDTHOFTHEPADDLE - WIDTHOFTHEINTERVAL, HEIGHTHTOFTHEWINDOW / 2 - LENGTHOFTHEPADDLE/2);
	ball.setPosition(LENGTHOFTHEWINDOW/2, HEIGHTHTOFTHEWINDOW/2);
	ball.normalizationVelocity.x = 0;
	ball.normalizationVelocity.y = 0;
}

void leftCollisionDectect(Ball &ball, Paddle &paddleOne,bool changeAngle,bool speedUpEachBounce,bool *leftHitLast,bool* touchedPaddle) {
	sf::Vector2f positionOfThePaddle = paddleOne.getPosition();
	sf::Vector2f positionOfTheBall = ball.getPosition();
	if (positionOfTheBall.x <= LEFTCOLLISIONLINE+RADIUSOFTHEBALL && positionOfTheBall.x > LEFTCOLLISIONLINE) {
		if (positionOfTheBall.y >= positionOfThePaddle.y && positionOfTheBall.y <= positionOfThePaddle.y + paddleOne.length) {
			if (changeAngle) {
				ball.normalizationVelocity.y = ball.normalizationVelocity.y * (positionOfTheBall.y - positionOfThePaddle.y - paddleOne.length / 2) / (paddleOne.length / 2);
				//angle will change according to the position that the ball hit on the paddle.
			}
			ball.normalizationVelocity.x = -ball.normalizationVelocity.x;
			soundOfHit.play();
			if (speedUpEachBounce) {
				ball.normalizationVelocity.x *= 1.05;
				ball.normalizationVelocity.y *= 1.05;
			}
			*leftHitLast = true;
			*touchedPaddle = true;
		}
		if ((positionOfTheBall.y < positionOfThePaddle.y && positionOfTheBall.y >= positionOfThePaddle.y - RADIUSOFTHEBALL)||
			(positionOfTheBall.y <= positionOfThePaddle.y + paddleOne.length + RADIUSOFTHEBALL && positionOfTheBall.y > positionOfThePaddle.y + paddleOne.length)) {
			//if the ball touch the angle of the paddle, it will get paddle's speed
			float xComponent = positionOfTheBall.x - LEFTCOLLISIONLINE;
			float yComponent = abs(positionOfTheBall.y - positionOfThePaddle.y - paddleOne.length / 2) - paddleOne.length / 2;
			if (xComponent*xComponent + yComponent*yComponent<= RADIUSOFTHEBALL * RADIUSOFTHEBALL){
				//the ball touched the paddle
				ball.normalizationVelocity.x = -ball.normalizationVelocity.x;
				ball.normalizationVelocity.x += paddleOne.speedOfPaddle.y * xComponent / RADIUSOFTHEBALL;
				ball.normalizationVelocity.y += paddleOne.speedOfPaddle.y * yComponent / RADIUSOFTHEBALL;
				soundOfHit.play();
				if (speedUpEachBounce) {
					ball.normalizationVelocity.x *= 1.05;
					ball.normalizationVelocity.y *= 1.05;
				}
				*leftHitLast = true;
				*touchedPaddle = true;
			}
		}
	}
	/*sf::Vector2f ballPositionOnCurrentFrame = ball.getPosition();
	sf::Vector2f ballPositionOnNextFrame;
	sf::Vector2f paddlePositionOnCurrentFrame = paddleOne.getPosition();
	sf::Vector2f paddlePositionOnNextFrame;
	ballPositionOnNextFrame.x = ballPositionOnCurrentFrame.x + ball.normalizationVelocity.x * timePerFrame;
	ballPositionOnNextFrame.y = ballPositionOnCurrentFrame.y + ball.normalizationVelocity.y * timePerFrame;
	paddlePositionOnNextFrame.y = paddlePositionOnCurrentFrame.y + paddleOne.speedOfPaddle.y * timePerFrame;
	float positionOnColisionLine = (LEFTCOLLISIONLINE - ballPositionOnNextFrame.x) * ball.normalizationVelocity.y / ball.normalizationVelocity.x + ballPositionOnNextFrame.y;
	if (positionOnColisionLine >= paddlePositionOnNextFrame.y && positionOnColisionLine <= paddlePositionOnNextFrame.y + paddleOne.length) {
		*leftHitLast = true;
		*touchedPaddle = true;
		sf::Vector2f truePositionOnNextFrame;
		truePositionOnNextFrame.x = LEFTCOLLISIONLINE * 2 - ballPositionOnNextFrame.x;
		truePositionOnNextFrame.y = ballPositionOnNextFrame.y;
	}*/

}

void rightCollisionDectect(Ball& ball, Paddle& paddleTwo, bool changeAngle,bool speedUpEachBounce, bool *leftHitLast,bool *touchedPaddle) {
	sf::Vector2f positionOfThePaddle = paddleTwo.getPosition();
	sf::Vector2f positionOfTheBall = ball.getPosition();
	if (positionOfTheBall.x >= RIGHTCOLLISIONLINE - RADIUSOFTHEBALL && positionOfTheBall.x < RIGHTCOLLISIONLINE) {
		if (positionOfTheBall.y >= positionOfThePaddle.y && positionOfTheBall.y <= positionOfThePaddle.y + paddleTwo.length) {
			if (changeAngle) {
				ball.normalizationVelocity.y = ball.normalizationVelocity.y * (positionOfTheBall.y - positionOfThePaddle.y - paddleTwo.length / 2) / (paddleTwo.length / 2);
			}
			ball.normalizationVelocity.x = -ball.normalizationVelocity.x;
			soundOfHit.play();
			if (speedUpEachBounce) {
				ball.normalizationVelocity.x *= 1.05;
				ball.normalizationVelocity.y *= 1.05;
			}
			*leftHitLast = false;
			*touchedPaddle = true;
		}
		if ((positionOfTheBall.y < positionOfThePaddle.y && positionOfTheBall.y >= positionOfThePaddle.y - RADIUSOFTHEBALL) ||
			(positionOfTheBall.y <= positionOfThePaddle.y + paddleTwo.length + RADIUSOFTHEBALL && positionOfTheBall.y > positionOfThePaddle.y + paddleTwo.length)) {
			float xComponent = RIGHTCOLLISIONLINE - positionOfTheBall.x;
			float yComponent = abs(positionOfTheBall.y - positionOfThePaddle.y - paddleTwo.length / 2) - paddleTwo.length / 2;
			if (xComponent * xComponent + yComponent * yComponent <= RADIUSOFTHEBALL * RADIUSOFTHEBALL) {
				ball.normalizationVelocity.x = -ball.normalizationVelocity.x;
				ball.normalizationVelocity.x += paddleTwo.speedOfPaddle.y * xComponent / RADIUSOFTHEBALL;
				ball.normalizationVelocity.y += paddleTwo.speedOfPaddle.y * yComponent / RADIUSOFTHEBALL;
				soundOfHit.play();
				if (speedUpEachBounce) {
					ball.normalizationVelocity.x *= 1.05;
					ball.normalizationVelocity.y *= 1.05;
				}
				*leftHitLast = false;
				*touchedPaddle = true;
			}
		}
	}
}

void showWinner(sf::RenderWindow& window,bool winner, sf::Clock &clock) {
	sf::Time showTime = clock.getElapsedTime() + sf::seconds(2);
	std::string winnerString = winner ? "Left" : "Right";
	std::string otherText = " side win the game";
	TextForPong showWinner(winnerString+otherText, FONTSIZEOFSCOREBOARD, sf::Color::White, sf::Vector2f(LENGTHOFTHEWINDOW / 2, HEIGHTHTOFTHEWINDOW/2), AlignOfText::center);
	
	while (clock.getElapsedTime() <= showTime) {
		window.clear();
		window.draw(showWinner);
		window.display();
	}
	window.clear();
}



int main()
{
	sf::RenderWindow windowForPong(sf::VideoMode(LENGTHOFTHEWINDOW, HEIGHTHTOFTHEWINDOW), "Pong!");
	bool gameStart = 0;
	bool newTurnStart = 0;
	int settingNumber = 1;
	int scoreOfTheRight = 0, scoreOfTheLeft = 0;
	Ball ball(sf::Color::White, RADIUSOFTHEBALL);
	Paddle paddleOne(sf::Color::White, WIDTHOFTHEPADDLE, LENGTHOFTHEPADDLE);
	Paddle paddleTwo(sf::Color::White, WIDTHOFTHEPADDLE, LENGTHOFTHEPADDLE);
	binarySetting playWithAI(settingNumber++, std::string("Play with AI?"));
	binarySetting changeAngle(settingNumber++, std::string("Can angle be various?"));
	binarySetting speedUpEachBounce(settingNumber++, std::string("Speed up each bounce?"));
	binarySetting powerUp(settingNumber++, std::string("Power up mode?"));
	bool powerUpexist = false;
	bool powerUpIsReady = false;
	bool ballTouchedPaddle = false;
	bool leftPlayerHitTheBallLast = false;
	sf::Vector2f centerOfThePowerUp;
	float radiusOfThePowerUp;
	sf::Time timeForGeneratePowerUp;
	sf::Vector2f positionOfPowerUp;
	buffer.loadFromFile("hit.wav");
	soundOfHit.setBuffer(buffer);
	sf::Clock generatorForPowerUp;
	TextForPong powerUpSymbol;

	while (windowForPong.isOpen())
	{
		float speedOfBallPerFrame = 0;
		float speedOfPaddlePerFrame = 0;
		bool firstFrame = 0;
		float timePerFrame = 0;
		sf::Clock clock;
		sf::Event event;
		
		bool gamePaused = 0;
		TextForPong textForStart(std::string("Press space to start"), FONTSIZEOFSETTING, sf::Color::White, sf::Vector2f(LENGTHOFTHEWINDOW / 2, HEIGHTHTOFTHEWINDOW / 2 + RADIUSOFTHEBALL), AlignOfText::center);
		float estimateYPosition = HEIGHTHTOFTHEWINDOW / 2;
		if (!gameStart) {
			TextForPong startGame(std::string("start"), FONTSIZEOFSCOREBOARD, sf::Color::White, sf::Vector2f(LENGTHOFTHEWINDOW / 2, HEIGHTHTOFTHEWINDOW - 12 * WIDTHOFTHEINTERVAL), AlignOfText::center, BorderShape::diamond);
			TextForPong clickToStart(std::string("Click to start"), FONTSIZEOFSETTING, sf::Color::White, sf::Vector2f(LENGTHOFTHEWINDOW / 2, HEIGHTHTOFTHEWINDOW - 7 * WIDTHOFTHEINTERVAL), AlignOfText::center);
			windowForPong.clear();
			playWithAI.draw(windowForPong);
			changeAngle.draw(windowForPong);
			speedUpEachBounce.draw(windowForPong);
			powerUp.draw(windowForPong);
			windowForPong.draw(startGame.borderShapeOfText);
			windowForPong.draw(startGame);
			windowForPong.draw(clickToStart);
			windowForPong.display();
			if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
				playWithAI.whetherSettingChanged(sf::Mouse::getPosition(windowForPong));
				changeAngle.whetherSettingChanged(sf::Mouse::getPosition(windowForPong));
				speedUpEachBounce.whetherSettingChanged(sf::Mouse::getPosition(windowForPong));
				powerUp.whetherSettingChanged(sf::Mouse::getPosition(windowForPong));
				if (startGame.getGlobalBounds().contains(sf::Mouse::getPosition(windowForPong).x, sf::Mouse::getPosition(windowForPong).y)) {
					gameStart = 1;
					windowForPong.clear();
					initializeTheGame(ball, paddleOne, paddleTwo);
					scoreOfTheLeft = 0; 
					scoreOfTheRight = 0;
				}
			}
		}
		if (gameStart) {
			
			if (powerUp.settingOn && ballTouchedPaddle) {
				if (!powerUpexist) {
					if (!powerUpIsReady) {
						std::default_random_engine generator(time(NULL));
						std::uniform_real_distribution<float> distribution1(0, 0.8 * LENGTHOFTHEWINDOW);
						std::uniform_real_distribution<float> distribution2(0, 0.8 * HEIGHTHTOFTHEWINDOW);
						std::uniform_real_distribution<float> distribution3(0, 3);
						positionOfPowerUp.x = distribution1(generator);
						positionOfPowerUp.y = distribution2(generator);
						timeForGeneratePowerUp = sf::seconds(distribution3(generator));	
						generatorForPowerUp.restart();
						powerUpIsReady = true;
					}
					if (powerUpIsReady && generatorForPowerUp.getElapsedTime() >= timeForGeneratePowerUp) {
						powerUpSymbol.initateTextForPong(std::string("<--->"), FONTSIZEOFSETTING*1.2, sf::Color::White, positionOfPowerUp, AlignOfText::center,BorderShape::circle);
						centerOfThePowerUp = powerUpSymbol.borderShapeOfText.getPosition();
						radiusOfThePowerUp = powerUpSymbol.borderShapeOfText.getRadius();
						powerUpexist = true;
						powerUpIsReady = false;
					}
				}
				if (powerUpexist) {
					float lengthOnX = abs(ball.getPosition().x - centerOfThePowerUp.x);
					float lengthOnY = abs(ball.getPosition().y - centerOfThePowerUp.y);
					float lengthOfTouch = ball.getRadius() + radiusOfThePowerUp;
					if (lengthOnX* lengthOnX + lengthOnY * lengthOnY <= lengthOfTouch*lengthOfTouch) {
						if (leftPlayerHitTheBallLast) {
							if (paddleOne.length <= LENGTHOFTHEWINDOW) {
								paddleOne.length += 50;
								paddleOne.setSize(sf::Vector2f(paddleOne.width, paddleOne.length));
							}
						}
						else {
							if (paddleTwo.length <= LENGTHOFTHEWINDOW) {
								paddleTwo.length += 50;
								paddleTwo.setSize(sf::Vector2f(paddleTwo.width, paddleTwo.length));
							}
						}
						powerUpexist = false;
					}
				}
			}
			TextForPong scoreboard(std::string(std::to_string(scoreOfTheLeft) + ":" + std::to_string(scoreOfTheRight)), FONTSIZEOFSCOREBOARD, sf::Color::White, sf::Vector2f(LENGTHOFTHEWINDOW / 2, WIDTHOFTHEINTERVAL), AlignOfText::center);
			timePerFrame = firstFrame?0:clock.restart().asSeconds();
			firstFrame = 0;
			speedOfBallPerFrame = SPEEDOFBALLPERSECOND * timePerFrame;
			speedOfPaddlePerFrame = SPEEDOFPADDLEPERSECOND * timePerFrame;
			ball.velocity.x = speedOfBallPerFrame * ball.normalizationVelocity.x;
			ball.velocity.y = speedOfBallPerFrame * ball.normalizationVelocity.y;
			while (windowForPong.pollEvent(event))
			{		
				if (event.type == sf::Event::Closed)
					windowForPong.close();
				if (event.type == sf::Event::KeyReleased) {
					if (event.key.code == sf::Keyboard::Space) {
						if (newTurnStart) {
							gamePaused = !gamePaused;
						}
						else {
							firstFrame = true;
							ballTouchedPaddle = false;
							powerUpexist = false;
							powerUpIsReady = false;
							paddleOne.length = LENGTHOFTHEPADDLE;
							paddleOne.setSize(sf::Vector2f(paddleOne.width, paddleOne.length));
							paddleTwo.length = LENGTHOFTHEPADDLE;
							paddleTwo.setSize(sf::Vector2f(paddleTwo.width, paddleTwo.length));
							newTurnStart = 1;
							ball.setARandomStartSpeed();
						}
					}
				}
				if (event.type == sf::Event::LostFocus)
					gamePaused = 1;
			}
			
			if (ball.velocity.x >= 0) {
				if (ball.getPosition().x >= RIGHTCOLLISIONLINE - RADIUSOFTHEBALL && ball.getPosition().x <= LENGTHOFTHEWINDOW) {
					rightCollisionDectect(ball, paddleTwo,changeAngle.settingOn,speedUpEachBounce.settingOn,&leftPlayerHitTheBallLast,&ballTouchedPaddle);
				}
				if (ball.getPosition().x > LENGTHOFTHEWINDOW) {
					initializeTheGame(ball, paddleOne, paddleTwo);
					newTurnStart = 0;
					scoreOfTheLeft += 1;
					scoreboard.updatescore(scoreOfTheLeft, scoreOfTheRight);
					if (scoreOfTheLeft >= WINSCORE) {
						showWinner(windowForPong, 1, clock);
						gameStart = false;
					}
				}
			}
			if (ball.velocity.x <= 0) {
				if (ball.getPosition().x <= LEFTCOLLISIONLINE + RADIUSOFTHEBALL && ball.getPosition().x > 0) {
					leftCollisionDectect(ball, paddleOne,changeAngle.settingOn, speedUpEachBounce.settingOn,&leftPlayerHitTheBallLast, &ballTouchedPaddle);
				}
				if (ball.getPosition().x < 0) {
					initializeTheGame(ball, paddleOne, paddleTwo);
					newTurnStart = 0;
					scoreOfTheRight += 1;
					scoreboard.updatescore(scoreOfTheLeft, scoreOfTheRight);
					if (scoreOfTheRight >= WINSCORE) {
						showWinner(windowForPong, 0 , clock);
						gameStart = false;
					}
				}
			}

			if (ball.getPosition().y <= RADIUSOFTHEBALL || ball.getPosition().y >= HEIGHTHTOFTHEWINDOW - RADIUSOFTHEBALL) {
				ball.velocity.y = -ball.velocity.y;
				ball.normalizationVelocity.y = -ball.normalizationVelocity.y;
			}
			
			if (!gamePaused) {
				paddleOne.controlPaddleByHuman(sf::Keyboard::isKeyPressed(sf::Keyboard::W), sf::Keyboard::isKeyPressed(sf::Keyboard::S),speedOfPaddlePerFrame);
				if (playWithAI.settingOn) {
					paddleTwo.controlPaddleByAI(ball, &estimateYPosition, speedOfPaddlePerFrame);
				}
				else {
					paddleTwo.controlPaddleByHuman(sf::Keyboard::isKeyPressed(sf::Keyboard::Up), sf::Keyboard::isKeyPressed(sf::Keyboard::Down),speedOfPaddlePerFrame);
				}
				ball.move(ball.velocity);
				windowForPong.clear();
				if (newTurnStart == 0) {
					windowForPong.draw(textForStart);
				}
				if (powerUp.settingOn && powerUpexist) {
					windowForPong.draw(powerUpSymbol.borderShapeOfText);
					windowForPong.draw(powerUpSymbol);
				}
				windowForPong.draw(ball);
				windowForPong.draw(paddleOne);
				windowForPong.draw(paddleTwo);
				windowForPong.draw(scoreboard);
				windowForPong.display();
			}	
		}
		while (windowForPong.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				windowForPong.close();
		}
	}
	return 0;
}
